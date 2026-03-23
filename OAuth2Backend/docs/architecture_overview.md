# OAuth2 系统架构总览 (Architecture Overview)

本文档提供整个 OAuth2 插件系统的全局视角，包括模块分层、授权码流程时序以及前后端协作方式。

---

## 1. 技术栈

| 层次 | 技术 | 用途 |
|---|---|---|
| **Web Framework** | [Drogon](https://github.com/drogonframework/drogon) | 高性能异步 C++ HTTP 框架 |
| **主数据库** | PostgreSQL 15 | 存储用户、角色、Token、Auth Code |
| **缓存层** | Redis | Token 快速验证 / 限流计数 / Auth Code 缓存 |
| **前端** | Vue 3 + Vite | SPA 客户端，发起 OAuth2 授权码流程 |
| **容器编排** | Docker Compose | 本地/生产环境一键部署全栈服务 |
| **监控** | Prometheus + Grafana | 实时指标采集与可视化 |

---

## 2. 模块分层图

```
┌─────────────────────────────────────────────────────┐
│                  HTTP Request                        │
└───────────────────────┬─────────────────────────────┘
                        │
         ┌──────────────▼──────────────┐
         │        Drogon Filter Layer  │
         │  ┌──────────────────────┐   │
         │  │ RateLimiterFilter    │   │  ← 限流 (Redis)
         │  ├──────────────────────┤   │
         │  │ OAuth2Middleware     │   │  ← Token 解析/注入属性
         │  ├──────────────────────┤   │
         │  │ AuthorizationFilter  │   │  ← RBAC 角色验证
         │  └──────────────────────┘   │
         └──────────────┬──────────────┘
                        │
         ┌──────────────▼──────────────┐
         │      Controller Layer       │
         │  OAuth2Controller           │  ← authorize / login / token / userinfo / register
         │  AdminController            │  ← /api/admin/dashboard
         │  GoogleController           │  ← /google/login (第三方登录代理)
         │  WeChatController           │  ← /api/wechat/login
         └──────────────┬──────────────┘
                        │
         ┌──────────────▼──────────────┐
         │        Service Layer        │
         │  AuthService                │  ← 用户注册/密码验证
         │  OAuth2CleanupService       │  ← 定时清理过期数据
         └──────────────┬──────────────┘
                        │
         ┌──────────────▼──────────────┐
         │        Plugin Layer         │
         │  OAuth2Plugin               │  ← 核心业务逻辑 (Token/Code 生命周期)
         │  OAuth2Metrics              │  ← Prometheus 指标收集
         └──────────────┬──────────────┘
                        │
         ┌──────────────▼──────────────┐
         │   Storage Abstraction Layer │
         │  IOAuth2Storage (接口)      │
         │  ├── MemoryOAuth2Storage    │  ← 开发/测试模式 (进程内)
         │  ├── PostgresOAuth2Storage  │  ← 生产主存储
         │  ├── RedisOAuth2Storage     │  ← 纯 Redis 模式
         │  └── CachedOAuth2Storage   │  ← 二级缓存 (Postgres + Redis Cache)
         └─────────────────────────────┘
```

---

## 3. 存储策略对比

`OAuth2Plugin` 支持通过 `config.json` 中 `storage_type` 字段切换存储后端：

| `storage_type` | 实现类 | 适用场景 |
|---|---|---|
| `memory` | `MemoryOAuth2Storage` | 单元测试 / 快速原型 |
| `redis` | `RedisOAuth2Storage` | 高吞吐无状态场景（需接受数据不持久化） |
| `postgres` | `PostgresOAuth2Storage` | 生产环境（数据持久化） |
| `cached_postgres` | `CachedOAuth2Storage` | 生产高性能（Postgres 主存 + Redis L2 Cache） |

---

## 4. OAuth2 授权码流程时序图

本项目实现标准 RFC 6749 授权码模式（Authorization Code Flow）：

```
浏览器 (Vue SPA)            OAuth2Backend              PostgreSQL / Redis
      │                         │                              │
      │  GET /oauth2/authorize   │                              │
      │ ─────────────────────► │                              │
      │                         │  (无 Session，渲染登录页)     │
      │ ◄─────────────────────  │                              │
      │                         │                              │
      │  POST /oauth2/login      │                              │
      │ ─────────────────────► │                              │
      │                         │  validateUser()              │
      │                         │ ────────────────────────────►│
      │                         │ ◄────────────────────────────│
      │                         │  generateAuthCode()          │
      │                         │ ────────────────────────────►│
      │                         │ ◄────────────────────────────│
      │  302 Location: /callback?code=XXX&state=YYY             │
      │ ◄─────────────────────  │                              │
      │                         │                              │
      │  POST /oauth2/token      │                              │
      │ ─────────────────────► │                              │
      │                         │  consumeAuthCode() [原子]    │
      │                         │ ────────────────────────────►│
      │                         │ ◄────────────────────────────│
      │                         │  saveAccessToken()           │
      │                         │ ────────────────────────────►│
      │  200 {access_token,…}   │ ◄────────────────────────────│
      │ ◄─────────────────────  │                              │
      │                         │                              │
      │  GET /oauth2/userinfo    │                              │
      │  Authorization: Bearer  │                              │
      │ ─────────────────────► │                              │
      │                         │  validateAccessToken()       │
      │                         │ ────────────────────────────►│ (Redis hit or Postgres)
      │                         │ ◄────────────────────────────│
      │  200 {sub,name,email}   │                              │
      │ ◄─────────────────────  │                              │
```

---

## 5. 前后端协作方式

```
┌──────────────────────┐        ┌──────────────────────────┐
│  OAuth2Frontend       │        │  OAuth2Backend (Drogon)   │
│  Vue 3 + Vite        │        │  Port: 5555               │
│  Port: 5173 (dev)    │        │                           │
│  Port: 80 (prod)     │        │                           │
│                      │        │  /oauth2/authorize        │
│  Login Page          │───────►│  /oauth2/login            │
│  Callback Handler    │◄───────│  /oauth2/token            │
│  Dashboard           │        │  /oauth2/userinfo         │
│                      │        │  /api/admin/...           │
└──────────────────────┘        └──────────────────────────┘
         │ Nginx (prod)
         │ - 静态文件托管 (Vue build)
         │ - 反向代理 /api → :5555
         │ - SSL 终结
```

生产环境建议在 Nginx 层完成 SSL 终结，并将 API 请求代理至 Drogon 后端，避免暴露后端端口。

---

## 6. 关键设计原则

- **异步优先**: 所有 I/O 操作（DB/Redis）全部使用 Drogon 异步回调，严禁阻塞 Event Loop。
- **存储解耦**: 通过 `IOAuth2Storage` 接口，存储后端可在不修改业务代码的情况下替换或组合。
- **防重放**: Auth Code 采用原子消费（`consumeAuthCode`）设计，物理上杜绝双花问题。
- **安全分层**: Filter → Plugin → Storage 三层独立校验，任一层均可拦截异常请求。
