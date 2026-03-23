# 测试策略与执行指南 (Testing Guide)

本文档说明项目的测试分层策略、各测试文件的覆盖范围，以及如何在本地执行全套测试。

---

## 1. 测试前置要求

在运行测试前，请确保以下服务已就绪：

| 服务 | 地址 | 说明 |
|---|---|---|
| **PostgreSQL** | `localhost:5432` | 数据库名: `oauth_test` / 用户: `test` / 密码: `123456` |
| **Redis** | `localhost:6379` | 密码: `123456`（与 `config.json` 一致）|

> 💡 **快速启动基础设施**：如果你使用 Docker，可以单独启动 postgres 和 redis 容器:
> ```powershell
> docker run -d -p 5432:5432 -e POSTGRES_USER=test -e POSTGRES_PASSWORD=123456 -e POSTGRES_DB=oauth_test postgres:15-alpine
> docker run -d -p 6379:6379 redis:alpine redis-server --requirepass 123456
> ```

---

## 2. 测试分层

### Level 1 — 单元测试（无网络 I/O）

| 测试文件 | 覆盖范围 |
|---|---|
| `MemoryStorageTest.cc` | 验证 `MemoryOAuth2Storage` 的所有 CRUD 接口（Auth Code、Token、Refresh Token 等），无需外部依赖 |
| `ConfigTest.cc` | 验证 `config.json` 的正确加载、RBAC 规则解析及插件配置读取 |
| `EnvConfigTest.cc` | 验证 `loadConfigWithEnv()` 函数能正确将环境变量注入 JSON 配置（`EnvInjectionVerify` 测试） |
| `RateLimiterTest.cc` | 验证 `RateLimiterFilter` 的路径匹配逻辑和 IP 提取策略 |

### Level 2 — 集成测试（需要 Redis / PostgreSQL）

| 测试文件 | 覆盖范围 | 依赖 |
|---|---|---|
| `RedisStorageTest.cc` | 验证 `RedisOAuth2Storage` 与真实 Redis 的交互（SETEX/GET/原子 Lua 脚本）| Redis |
| `PostgresStorageTest.cc` | 验证 `PostgresOAuth2Storage` 与真实 Postgres 的交互（Auth Code/Token CRUD）| Postgres |
| `AdvancedStorageTest.cc` | 验证已撤销/已过期 Token 的拒绝逻辑，以及并发场景下的数据正确性 | Redis / Postgres |
| `UserTest.cc` | 验证用户注册、密码验证（`AuthService`）及 RBAC 角色查询全流程 | Postgres |
| `PluginTest.cc` | 验证 `OAuth2Plugin` 核心业务流程，包括：`TestReplayAttack`（防重放）、完整 Code→Token 交换、客户端校验 | Postgres / Redis |

### Level 3 — 端到端集成测试

| 测试文件 | 覆盖范围 | 依赖 |
|---|---|---|
| `IntegrationE2ETest.cc` | 模拟完整 OAuth2 授权码流程：HTTP 请求 → 授权 → 登录 → 换 Token → UserInfo 验证 | Postgres + Redis + 运行中的 Drogon App |

---

## 3. 执行方式

### 方式一：通过 CTest（推荐）

```powershell
# 在构建完成后执行
cd OAuth2Backend\build
ctest -V -C Release --output-on-failure --timeout 120
```

### 方式二：直接运行测试可执行文件

测试可执行文件内部会自动启动 Drogon App 实例（`test_main.cc` 中通过信号量同步），**无需手动启动 OAuth2Server**。

```powershell
cd OAuth2Backend\build\test\Release
.\OAuth2Test_test.exe
```

### 方式三：使用 Workflow

```powershell
# 执行完整的单元测试和集成测试
/test

# 执行包含数据库重置的完整 E2E 流程
/test-e2e
```

---

## 4. 测试输出示例

```
All tests passed (54 assertions in 11 tests)
```

如果出现失败，失败的测试名称和断言位置会被打印：
```
In test case RedisStorageTest
  RedisStorageTest.cc:63  FAILED:
    CHECK(c.has_value())
```

**常见失败原因**：
- Redis 或 PostgreSQL 服务未启动 → 检查服务是否可达
- Redis 密码不匹配 → 检查 `config.json` 中的 `passwd` 字段
- 数据库未初始化 → 执行 `sql/` 目录下的 SQL 脚本

---

## 5. CI 中的测试

每次 Push 到 `master` 或发起 PR 时，GitHub Actions CI 会自动执行：

1. 启动 Postgres 和 Redis Service Container
2. 初始化数据库 Schema  
3. 编译项目
4. 运行 `ctest`

详见 [CI/CD 指南](ci_cd_guide.md)。
