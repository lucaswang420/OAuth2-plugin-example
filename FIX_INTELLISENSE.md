# 修复 IntelliSense 红色波浪线问题

## 问题原因
IntelliSense 缓存了错误的路径指向 VS Code 扩展目录，导致无法找到系统头文件如 `cstddef`。

## 已完成的修复

1. **添加了系统包含路径** 到 c_cpp_properties.json
2. **修复了 Windows SDK 版本** 从 10.0.26200.0 到 10.0.26100.0
3. **配置了 MSVC 编译器路径** 为完整路径
4. **添加了 IntelliSense 缓存路径** 配置

## 立即执行以下步骤

### 1. 清除 IntelliSense 缓存
按 `Ctrl+Shift+P`，输入以下命令并执行：
- `C/C++: Reset IntelliSense Database`
- 选择 "Windows" 配置

### 2. 重新加载窗口
按 `Ctrl+Shift+P`，输入：
- `Developer: Reload Window`

### 3. 如果红色波浪线仍然存在
完全重启 VS Code：
1. 关闭 VS Code
2. 重新打开项目
3. 等待 IntelliSense 完成索引（查看右下角状态栏）

### 4. 验证配置
1. 打开任意 .cc 或 .h 文件
2. 按 `Ctrl+Shift+P`
3. 输入 `C/C++: Log Diagnostics`
4. 检查输出中的 "Include Path" 是否包含系统路径

## 手动验证包含路径

检查输出是否包含：
- ✅ MSVC 包含路径：`.../MSVC/14.44.35207/include`
- ✅ Windows SDK：`.../Windows Kits/10/Include/10.0.26100.0/ucrt`
- ✅ Drogon：`.../drogon/include`
- ✅ PostgreSQL：`.../PostgreSQL/13/include`

## 如果问题仍然存在

1. **检查 C/C++ 扩展版本**：
   - 确保安装了最新版本的 C/C++ 扩展
   - 当前建议版本：1.31.4 或更高

2. **重新构建项目**：
   ```bash
   cd OAuth2Backend
   cmake --build build --clean-first
   ```

3. **检查 CMake 配置**：
   - 按 `Ctrl+Shift+P`
   - 执行 `CMake: Configure`
   - 确保没有错误

## 成功标志

配置成功后，你应该看到：
- ✅ 没有红色波浪线
- ✅ 可以正常跳转到 Drogon 类定义
- ✅ 系统头文件如 `<iostream>`, `<cstddef>` 等可以正常识别
- ✅ 代码补全正常工作