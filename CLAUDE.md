# OAuth2 Plugin Example - Claude Code 项目规范

> 本文档为 Claude Code 提供 OAuth2 项目的开发指导，技术规范请参考 [TECH_SPECS.md](TECH_SPECS.md)。

---

## 项目概述

**Drogon OAuth2.0 Provider & Vue Client Demo** - 功能完整的 OAuth2.0 授权服务器，支持本地认证和外部提供商（微信）集成。

**技术栈**: Drogon C++17 | PostgreSQL 14+ (主存储) | Redis 7+ (缓存) | Vue.js 3 | CMake 3.20+ | Conan (Windows)

**项目结构**: `controllers/` (HTTP层) → `filters/` (中间件) → `plugins/` (核心业务) → `services/` (服务层) → `storage/` (数据层) → `models/` (ORM，禁止修改)

---

## 技术规范

本项目遵循 [Drogon OAuth2 技术规范](TECH_SPECS.md)：架构规范、数据访问、代码质量、安全规范

**重要**: 禁止修改 ORM 类（用 `drogon_ctl` 重新生成） | 禁止 raw SQL（特殊情况除外） | 优先异步回调，禁止协程 | Lambda 捕获 `[sharedCb]`

---

## 配置管理

**配置文件**: `config.json` (主) | `config.{dev,ci,prod}.json` (环境特定)

**敏感信息**: 使用环境变量 `OAUTH2_DB_PASSWORD`, `OAUTH2_REDIS_PASSWORD`

---

## 测试规范

| 测试类型 | 命令 | 说明 |
|----------|------|------|
| 单元测试 | `cd build && ctest` | 每次代码变更，覆盖率 80%+，< 30 秒 |
| 集成测试 | `ctest -R Integration` (Linux) / `ctest -R Memory` (Windows) | API 接口级验证 |
| 完整测试 | `scripts/full_test.bat` | 包含数据库、ORM 生成、编译、测试 |
| 失败处理 | 同一错误重复 3 次停止，分析根本原因 | |

---

## 构建部署

**Windows**: `scripts/build.bat` [-debug] | `scripts/test.bat` | `scripts/run_server.bat`

**Linux/macOS**: `scripts/build.sh` [Debug] [--build-drogon] | `cd build && ctest` | `./OAuth2Server -c config.json`

详细构建指南: [scripts/README.build.md](OAuth2Backend/scripts/README.build.md)

**跨平台 CI/CD**: Linux (Ubuntu 22.04, GCC + PostgreSQL + Redis) | Windows (Server 2022, MSVC 2022 + 内存存储) | macOS (14, Clang + ARM64 构建验证)

---

## 开发流程

- [+] 允许 `git commit` | [-] 禁止 `git push` (需人工审核)
- 调试代码问题解决后必须移除，使用 `LOG_DEBUG` 条件日志
- **完成标准**: 测试通过 + 静态分析通过 + 跨平台 CI 成功 + 文档更新

---

## 项目架构要点

| 模块 | 说明 |
|------|------|
| OAuth2 流程 | 授权码: `/oauth/authorize` → `/oauth/callback` → `/oauth/token` \| Token 刷新: `/oauth/token?grant_type=refresh_token` \| 用户认证: `/auth/login` |
| RBAC 系统 | users ↔ user_roles ↔ roles ↔ role_permissions ↔ permissions \| `RolePermissionFilter` 自动验证 \| Redis 缓存用户角色 |
| 存储策略 | 生产: `CachedOAuth2Storage(Postgres + Redis)` \| 测试: `MemoryOAuth2Storage` \| 开发: `PostgresOAuth2Storage` / `RedisOAuth2Storage` |
| 外部登录 | 微信: `WeChatController` \| Google: `GoogleController` (示例) \| 扩展参考 `OAuth2Plugin.validateClient` |

---

**文档版本**: v3.0 | **最后更新**: 2026-05-12 | **维护者**: OAuth2 Plugin 开发团队
