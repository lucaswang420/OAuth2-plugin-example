# Settings.json 配置说明

## IDE 警告说明

如果您在IDE中看到类似 "Missing property 'hooks'" 的警告，这是**误报**，可以安全忽略。

### 验证配置正确性

```bash
# 验证JSON格式
python -m json.tool .claude/settings.json
```

### 当前配置功能

✅ **自动格式化**: 编辑C++代码后自动运行clang-format
✅ **提交前测试**: git commit前自动运行测试套件
✅ **权限管理**: 允许使用Edit、Write、Read、Bash、Glob、Grep工具

### 警告原因

这个警告是由于IDE的JSON schema验证器与实际的Claude Code配置格式不完全匹配导致的。虽然IDE显示警告，但配置文件完全正常工作。

### 如果警告困扰您

1. 配置文件功能正常，可以安全使用
2. 可以在IDE中禁用此文件的schema验证
3. 或者忽略这个警告，不影响实际功能
