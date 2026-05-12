# Drogon OAuth2 技术规范

> 本文档定义 Drogon OAuth2 项目的技术规范，包括架构设计、编码标准、安全要求等。

---

## 一、架构规范

### [MUST] Drogon 框架优先原则
- 优先使用 Drogon 内置功能，避免引入三方库
- 引入新库必须在 PR 中说明必要性

### [MUST] 分层架构

| 层级 | 职责 | 关键要求 |
|------|------|----------|
| Controller 层 | HTTP 请求/响应 | 薄层设计，验证格式，调用 Plugin/Service |
| Plugin/Service 层 | 核心业务逻辑 | Plugin 模式，依赖注入，单例管理 |
| Storage 层 | 数据访问 | `IOAuth2Storage.h` 接口，Strategy 模式 |
| Model 层 | ORM 映射 | 禁止修改 ORM 类，用 `drogon_ctl` 重新生成 |

### [MUST] 异步编程规范

| 接口类型 | 优先级 | 说明 |
|----------|--------|------|
| 异步回调 | [+] 最高 | `Mapper::findOne`, `execSqlAsync` |
| 同步接口 | [!] 限制 | `Mapper::findBy` with future（非必要禁止） |
| 协程接口 | [-] 禁止 | `CoroMapper`（严格禁止使用） |

**Lambda 捕获规范**:
- [+] 捕获 `sharedCb`: `[sharedCb]`
- [-] 捕获裸指针: `[this]`, `[&var]`
- 如需使用裸指针，必须在 PR 中说明生命周期保障方案 (`shared_from_this`, `weak_ptr`)

---

## 二、数据访问规范

### [MUST] ORM 使用规范

| 操作 | 禁止 | 推荐 |
| :--- | :--- | :--- |
| SELECT | raw SQL | `Mapper::findBy` |
| INSERT | raw INSERT | `Mapper::insert` |
| UPDATE | raw UPDATE | `Mapper::update` |
| JOIN | JOIN 查询 | 拆分查询或 `Criteria::In` |

**允许使用 raw SQL 的特殊情况**:
- [+] PostgreSQL `UPDATE ... RETURNING` (原子操作)
- [+] DDL 操作 (表结构变更，需用 SchemaSetup.cc)
- [+] 批量操作优化 (需说明必要性)
- [+] 测试代码清理

### 关键规范

| 规范项 | 要求 |
|--------|------|
| Callback 生命周期 | 使用 `std::make_shared<CallbackType>(std::move(cb))` |
| 替代 JOIN | 拆分为多个 ORM 查询或使用 `Criteria::In` |
| 错误处理 | 所有异步回调都有错误处理分支 |
| Lambda 捕获 | 捕获 `[sharedCb]` 而非裸指针 |

### [MUST] 数据库连接管理
- 读写分离: `dbClientMaster_` (写), `dbClientReader_` (读)
- 连接池配置在 `config.json` 中
- 异步操作使用共享的 DbClientPtr

---

## 三、代码质量规范

### [MUST] 代码风格

| 规范项 | 要求 |
|--------|------|
| 语言标准 | C++17 |
| 风格指南 | Google C++ Style Guide (Drogon 默认) |
| 行长度限制 | 100 字符 |
| 格式化工具 | clang-format 自动格式化 |
| 字符规范 | 禁止 emoji，使用 ASCII 符号如 `[+]`, `[-]`, `[!]`（Windows 兼容性） |

### [MUST] 错误处理

| 错误类型 | 处理要求 |
|----------|----------|
| Drogon 异常 | 必须捕获: `catch (const DrogonDbException &e)` |
| 异步回调失败 | 必须在失败时调用 `(*sharedCb)(errorResult)` |
| 日志级别 | `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR` |

### [MUST] 性能优化

| 优化项 | 要求 |
|--------|------|
| 接口选择 | 优先使用异步接口，避免阻塞 |
| 缓存策略 | 合理使用缓存 (CachedOAuth2Storage) |
| 数据库优化 | 使用索引，避免 N+1 查询 |
| 连接池配置 | 根据并发需求调整 |

---

## 四、安全规范

### [MUST] 输入验证

| 验证项 | 要求 |
|--------|------|
| 用户输入 | 所有用户输入必须验证 |
| SQL 查询 | 使用 ORM Criteria，禁止字符串拼接 |
| XSS 防护 | 使用 Drogon 内置 CSP 和模板转义 |

### [MUST] 认证授权

| 规范项 | 要求 |
|--------|------|
| OAuth2 流程 | 严格遵守 RFC 6749 |
| Token 有效期 | Access Token (1h), Refresh Token (30d) |
| 权限检查 | 所有受保护端点必须验证权限 |

### [MUST] 敏感数据保护

| 数据类型 | 保护要求 |
|----------|----------|
| 密码 | 使用 SHA-256 + salt 哈希 |
| Client Secret | 使用 SHA-256 + salt 存储 |
| 日志输出 | 禁止日志中输出敏感信息 (密码, token) |

---

**文档版本**: v2.0 | **最后更新**: 2026-05-12 | **维护者**: OAuth2 Plugin 开发团队
