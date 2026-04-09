# Claude Code 自动化配置记忆

这是为OAuth2插件示例项目配置的Claude Code自动化设置。

## 项目概况

- C++ OAuth2后端 (Drogon框架) + Vue.js前端
- 约1.8万行C++代码
- 使用CMake + Conan构建
- GitHub Actions CI/CD
- OpenAPI 3.0文档

## 已配置的自动化

### 🔌 MCP服务器

- **context7**: ✅ 已安装并连接 - 为Drogon、C++、Vue.js提供最新文档
- **github**: ⚠️ 已配置但需要认证 - GitHub集成，管理PR和Issues（需要手动认证）

### ⚡ Hooks

- **自动格式化**: C++代码编辑后自动运行clang-format
- **提交前测试**: git commit前运行测试，测试通过后才允许提交

### 🎯 技能

- **build-and-test**: 简化构建和测试流程
- **openapi-update**: 保持API文档与代码同步

### 🤖 子代理

- **code-reviewer**: 专注OAuth2实现的代码审查（手动触发）
- **api-documenter**: 自动维护OpenAPI规范

## 使用方式

### 技能调用

```bash
# 构建和测试
/build-and-test

# 更新API文档
/openapi-update
```

### 自动触发

- C++文件编辑 → 自动格式化
- git commit → 运行测试，测试通过后才允许提交
- API端点变更 → api-documenter更新文档

### 手动触发

- **代码审查**: 在对话中请求 "请审查这段代码" 或 "使用code-reviewer检查安全性"
- **构建测试**: 使用 `/build-and-test` 命令
- **API文档更新**: 使用 `/openapi-update` 命令

## 配置文件位置

- 主配置: `.claude/settings.json`
- 技能: `.claude/skills/*/SKILL.md`
- 子代理: `.claude/agents/*.md`

## 维护说明

- 定期检查hook输出是否正常
- 更新技能和代理以匹配项目变化
- 考虑团队需求添加更多自动化
