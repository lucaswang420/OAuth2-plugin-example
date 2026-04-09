---
name: openapi-update
description: 当OAuth2端点发生变化时更新OpenAPI规范
---

# OpenAPI规范更新技能

这个技能帮助您在OAuth2控制器端点发生变化时更新OpenAPI 3.0规范文档。

## 使用方法

- Claude自动调用：当检测到OAuth2Controller.cc或WeChatController.cc中的路由变更时
- 用户调用：`/openapi-update`

## 工作流程

1. **分析当前控制器**
   - 读取`OAuth2Backend/controllers/OAuth2Controller.cc`
   - 读取`OAuth2Backend/controllers/WeChatController.cc`
   - 识别所有路由端点和参数

2. **比较现有OpenAPI规范**
   - 读取`OAuth2Backend/openapi.yaml`
   - 检查是否有新的端点
   - 检查是否有参数变更
   - 检查是否有响应格式变更

3. **更新OpenAPI规范**
   - 添加新的端点定义
   - 更新现有端点的参数
   - 更新响应模型
   - 确保符合OpenAPI 3.0规范

4. **验证规范**
   - 检查YAML语法正确性
   - 验证所有引用是否有效
   - 确保端点路径与代码一致

## 需要检查的关键端点

### OAuth2标准端点
- `GET /oauth2/authorize` - 授权端点
- `POST /oauth2/token` - 令牌端点
- `POST /oauth2/revoke` - 撤销端点
- `GET /oauth2/verify` - 验证端点

### WeChat集成端点
- `GET /api/wechat/login` - 微信登录
- `GET /api/wechat/callback` - 微信回调

### 管理端点
- `GET /api/clients` - 客户端管理
- `POST /api/clients` - 创建客户端
- `PUT /api/clients/{id}` - 更新客户端
- `DELETE /api/clients/{id}` - 删除客户端

## 输出格式

更新后的`openapi.yaml`文件应包含：
- 正确的OpenAPI 3.0版本
- 所有端点的完整文档
- 请求参数 schema
- 响应格式定义
- 错误响应示例
- 认证方式说明

## 注意事项

- 保持YAML缩进一致（2个空格）
- 所有端点需要包含描述文字
- 参数需要标注是否必需
- 提供请求和响应示例
- 更新版本号当有重大变更
