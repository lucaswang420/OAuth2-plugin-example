# VS Code C++ IntelliSense 配置完成

## 已完成的配置

1. **创建了 `c_cpp_properties.json`** - C++ IntelliSense 配置文件
2. **更新了 `settings.json`** - 添加了 C++ 相关设置
3. **创建了 `launch.json`** - 调试配置
4. **创建了 `tasks.json`** - 构建任务配置

## 下一步操作

### 1. 重新加载 VS Code 窗口
请按以下步骤重新加载 VS Code 以使配置生效：
- 按 `Ctrl+Shift+P` 打开命令面板
- 输入 `Developer: Reload Window`
- 按 Enter 执行

### 2. 验证 C++ 扩展
确保已安装以下扩展：
- **C/C++** (by Microsoft)
- **CMake Tools** (by Microsoft)

### 3. 测试跳转功能
重新加载后：
1. 打开 `OAuth2Backend/main.cc`
2. 右键点击任何 Drogon 类（如 `drogon::app()`）
3. 选择 "Go to Definition" 或按 `F12`

### 4. 如果仍有问题
如果跳转仍然不工作，请尝试：
- 重启 VS Code
- 检查 C/C++ 扩展是否已启用
- 运行 `CMake: Configure` 命令
- 检查输出面板的 "C/C++" 和 "CMake" 频道是否有错误信息

## 配置详情

- **C++ 标准**: C++17
- **编译器**: MSVC (Visual Studio 2022)
- **IntelliSense 模式**: windows-msvc-x64
- **包含路径**: 已配置项目路径和依赖库路径
- **跨平台支持**: 使用 `${workspaceFolder}` 和环境变量，支持不同电脑

## 跨平台/跨电脑配置说明

所有配置文件都使用相对路径和环境变量，可以在不同电脑上使用：

1. **自动适应的路径**:
   - `${workspaceFolder}` - 自动指向工作区根目录
   - `${env:ProgramFiles}` - 自动使用系统 Program Files 路径
   - `${env:USERPROFILE}` - 自动使用用户主目录

2. **个性化配置**:
   - 如果需要自定义路径，复制 `.vscode/settings.local.json.example` 为 `settings.local.json`
   - `settings.local.json` 已被 `.gitignore` 忽略，不会被提交到版本控制

3. **团队协作**:
   - 所有开发者共享相同的 `.vscode/settings.json` 和 `.vscode/c_cpp_properties.json`
   - 个人设置放在 `settings.local.json` 中

配置完成后，你应该能够：
- 跳转到定义 (F12)
- 查看引用 (Shift+F12)
- 代码补全
- 语法检查
- 调试应用程序