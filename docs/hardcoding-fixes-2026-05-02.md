# Hardcoding Fixes - 2026-05-02

本文档记录了项目中三个硬编码问题的修复方案和使用说明。

## 修复概述

### P1: 前端第三方登录配置硬编码

**问题**: [Login.vue:62-76](OAuth2Frontend/src/views/Login.vue) 中微信和 Google 的 client_id、redirect_uri 都是硬编码的占位符。

**修复**: 创建了前端配置管理系统，支持环境变量和运行时配置。

### P2: OpenAPI 生成器服务地址硬编码

**问题**: [OpenApiGenerator.cc:48](OAuth2Backend/common/documentation/OpenApiGenerator.cc) 固定使用 `http://localhost:5555` 作为服务器地址。

**修复**: 从 Drogon 配置读取实际监听地址，或使用相对路径。

### P2: 内存存储管理员身份硬编码

**问题**: [MemoryOAuth2Storage.h:61-69](OAuth2Backend/storage/MemoryOAuth2Storage.h) 硬编码了 `userId == "1" || userId == "admin"` 为管理员。

**修复**: 改为可配置的权限规则，支持从配置文件读取管理员用户。

---

## 使用说明

### 1. 前端第三方登录配置

#### 方法一：环境变量（推荐用于生产环境）

创建 `.env` 文件：

```bash
# OAuth2 Configuration
VITE_OAUTH2_CLIENT_ID=vue-client

# WeChat OAuth2 Configuration
VITE_WECHAT_ENABLED=true
VITE_WECHAT_APPID=wx1234567890abcdef
VITE_WECHAT_REDIRECT_URI=http://your-domain.com/callback

# Google OAuth2 Configuration
VITE_GOOGLE_ENABLED=true
VITE_GOOGLE_CLIENT_ID=123456789-abcdefg.apps.googleusercontent.com
VITE_GOOGLE_REDIRECT_URI=http://localhost:5173/callback

# API Configuration
VITE_API_BASE_URL=
```

#### 方法二：运行时配置（推荐用于快速部署）

1. 复制 `public/config.example.json` 为 `public/config.json`
2. 修改配置文件中的实际凭证：

```json
{
  "oauth2": {
    "clientId": "vue-client",
    "authorizeEndpoint": "/oauth2/authorize",
    "tokenEndpoint": "/oauth2/token",
    "scope": "openid profile"
  },
  "providers": {
    "wechat": {
      "enabled": true,
      "appId": "wx1234567890abcdef",
      "redirectUri": "http://your-domain.com/callback",
      "authUrl": "https://open.weixin.qq.com/connect/qrconnect",
      "scope": "snsapi_login"
    },
    "google": {
      "enabled": true,
      "clientId": "123456789-abcdefg.apps.googleusercontent.com",
      "redirectUri": "http://localhost:5173/callback",
      "authUrl": "https://accounts.google.com/o/oauth2/v2/auth",
      "scope": "openid email profile"
    }
  },
  "app": {
    "apiBaseUrl": "",
    "callbackPath": "/callback"
  }
}
```

**优先级**: 环境变量 > 运行时配置 > 默认值

### 2. OpenAPI 服务器地址配置

OpenAPI 生成器现在自动从 Drogon 配置读取服务器地址：

- **开发环境**: 自动使用 `config.json` 中配置的监听地址
- **生产环境**: 根据实际部署的服务器地址生成文档
- **默认行为**: 如果无法确定服务器地址，使用相对路径 `/`

无需手动配置，系统会在启动时自动生成正确的 OpenAPI 文档。

查看生成的文档：
```bash
# 文档位置
build/docs/api/openapi.json

# 或访问 Swagger UI
http://localhost:5555/docs/api/swagger-ui/
```

### 3. 内存存储管理员配置

在 `config.json` 的 OAuth2Plugin 配置节添加 `admin_users` 字段：

```json
{
  "plugins": [
    {
      "name": "OAuth2Plugin",
      "dependencies": [],
      "config": {
        "storage_type": "memory",
        "clients": {
          "vue-client": {
            "client_type": "PUBLIC",
            "secret": "123456",
            "redirect_uri": "http://localhost:5173/callback"
          }
        },
        "admin_users": {
          "admin": ["admin", "user"],
          "1": ["admin", "user"],
          "test_admin": ["admin", "user", "moderator"]
        }
      }
    }
  ]
}
```

**配置说明**:
- `admin_users`: 对象，key 为用户ID，value 为角色数组
- 支持多角色分配：`["admin", "user", "moderator"]`
- 支持单角色字符串：`"admin"`
- 未配置的用户默认获得 `["user"]` 角色

**向后兼容**: 如果不配置 `admin_users`，系统会使用默认配置（`admin` 和 `1` 为管理员）。

---

## 测试验证

### 前端配置测试

1. **环境变量测试**:
   ```bash
   cd OAuth2Frontend
   cp .env.example .env
   # 编辑 .env 文件，填入实际凭证
   npm run dev
   ```

2. **运行时配置测试**:
   ```bash
   cd OAuth2Frontend/public
   cp config.example.json config.json
   # 编辑 config.json，填入实际凭证
   cd ..
   npm run dev
   ```

3. **验证**:
   - 打开浏览器开发者工具 → Console
   - 查看是否有 "Runtime configuration loaded successfully" 消息
   - 尝试登录，检查是否使用了配置的回调地址

### OpenAPI 配置测试

1. **启动后端**:
   ```bash
   cd OAuth2Backend/build
   ./OAuth2Backend -c config.json
   ```

2. **验证生成的文档**:
   ```bash
   cat docs/api/openapi.json | grep servers
   ```

   应该看到类似：
   ```json
   "servers": [
     {
       "url": "http://0.0.0.0:5555",
       "description": "OAuth2 Authorization Server"
     }
   ]
   ```

3. **访问 Swagger UI**:
   ```
   http://localhost:5555/docs/api/swagger-ui/
   ```

### 管理员配置测试

1. **修改配置**:
   ```json
   "admin_users": {
     "test_user": ["admin", "user"],
     "regular_user": ["user"]
   }
   ```

2. **启动后端**:
   ```bash
   cd OAuth2Backend/build
   ./OAuth2Backend -c config.json
   ```

3. **测试权限**:
   ```bash
   # 使用 test_user 登录，应该能访问 /api/admin/* 端点
   # 使用 regular_user 登录，应该被拒绝访问 /api/admin/* 端点
   ```

---

## 文件变更清单

### 新增文件

- `OAuth2Frontend/src/config/auth.config.js` - 前端配置管理
- `OAuth2Frontend/public/config.example.json` - 运行时配置示例
- `OAuth2Frontend/.env.example` - 环境变量示例

### 修改文件

- `OAuth2Frontend/src/views/Login.vue` - 使用配置系统
- `OAuth2Backend/common/documentation/OpenApiGenerator.h` - 添加服务器配置接口
- `OAuth2Backend/common/documentation/OpenApiGenerator.cc` - 从配置读取服务器地址
- `OAuth2Backend/main.cc` - 自动配置 OpenAPI 服务器地址
- `OAuth2Backend/storage/MemoryOAuth2Storage.h` - 添加用户角色存储
- `OAuth2Backend/storage/MemoryOAuth2Storage.cc` - 实现可配置权限
- `OAuth2Backend/plugins/OAuth2Plugin.cc` - 传递管理员配置
- `OAuth2Backend/config.json` - 添加 admin_users 示例

---

## 注意事项

### 安全建议

1. **敏感信息管理**:
   - 生产环境必须使用环境变量存储密钥
   - `.env` 文件应该添加到 `.gitignore`
   - `public/config.json` 不要包含生产凭证

2. **CORS 配置**:
   - 确保配置文件中的 `cors.allow_origins` 包含前端域名
   - 第三方登录回调地址必须在 OAuth 提供商处注册

3. **管理员权限**:
   - 内存存储仅用于开发/测试环境
   - 生产环境应使用 PostgreSQL + Redis 存储
   - 管理员用户应通过数据库进行 RBAC 管理

### 兼容性说明

- 前端配置系统向后兼容，未配置时使用默认值
- OpenAPI 生成器默认使用相对路径，兼容所有环境
- 管理员配置向后兼容，未配置时使用旧版硬编码规则

---

**文档版本**: v1.0
**最后更新**: 2026-05-02
**维护者**: OAuth2 Plugin 开发团队
