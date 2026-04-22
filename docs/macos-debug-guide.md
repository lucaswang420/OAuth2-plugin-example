# macOS 测试问题排查指南

## 背景说明

macOS CI 中的测试遇到了 `std::bad_function_call` 运行时错误，这是 Drogon 框架在 macOS 上与 C++17/20 的兼容性问题。测试已被禁用，但我们可以通过本地环境来排查和调试这个问题。

## 已知问题

根据 CI 配置（`.github/workflows/ci-macos.yml` 第 132-142 行）：

```yaml
# Temporarily disable tests due to std::bad_function_call runtime issue
# This is a Drogon framework-specific issue on macOS with C++17/20 compatibility
```

**症状**：
- ✅ 编译成功
- ❌ 运行测试时出现 `std::bad_function_call` 错误
- ⚠️ 这是 Drogon 框架层面的兼容性问题

## 快速开始

### 1. 运行本地验证脚本

```bash
# 在项目根目录运行
./macos-quick-verify.sh
```

这个脚本会自动：
1. ✅ 检查 Homebrew 和依赖
2. ✅ 启动 PostgreSQL 和 Redis 服务
3. ✅ 初始化数据库
4. ✅ 编译项目
5. ✅ 尝试运行测试
6. ✅ 捕获和分析错误

### 2. 查看详细错误日志

```bash
# 查看测试输出
cat OAuth2Backend/build/test_output.txt
```

## 手动调试步骤

如果自动化脚本无法解决问题，可以手动执行以下步骤：

### 步骤 1: 环境准备

```bash
# 安装依赖
brew install git cmake jsoncpp ossp-uuid zlib openssl@1.1 redis postgresql@14 hiredis

# 启动服务
brew services start postgresql@14
brew services start redis

# 等待服务就绪
sleep 5
```

### 步骤 2: 初始化数据库

```bash
# 创建数据库
psql -d postgres -c "CREATE DATABASE oauth_test;"
psql -d postgres -c "CREATE USER test WITH PASSWORD '123456';"
psql -d postgres -c "GRANT ALL PRIVILEGES ON DATABASE oauth_test TO test;"

# 初始化表
cd OAuth2Backend
export PGPASSWORD=123456

psql -h localhost -U test -d oauth_test -f sql/001_oauth2_core.sql
psql -h localhost -U test -d oauth_test -f sql/002_users_table.sql
psql -h localhost -U test -d oauth_test -f sql/003_rbac_schema.sql
```

### 步骤 3: 编译项目

```bash
cd OAuth2Backend

# 配置 CMake（强制 C++17）
cmake -S . -B build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_CXX_STANDARD_REQUIRED=ON \
  -DCMAKE_CXX_EXTENSIONS=OFF \
  -DCMAKE_CXX_FLAGS="-std=c++17" \
  -DBUILD_TESTS=ON \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@1.1)" \
  -DCMAKE_FIND_FRAMEWORK=LAST

# 编译
cmake --build build --config Release --parallel $(sysctl -n hw.ncpu)
```

### 步骤 4: 运行测试

```bash
cd build

# 设置环境变量
export OAUTH2_DB_HOST="127.0.0.1"
export OAUTH2_DB_NAME="oauth_test"
export OAUTH2_DB_PASSWORD="123456"
export OAUTH2_REDIS_HOST="127.0.0.1"
export OAUTH2_REDIS_PASSWORD=""

# 运行测试
./test/OAuth2Test_test
```

## 常见错误诊断

### 1. std::bad_function_call 错误

**症状**：
```
terminate called after throwing an instance of 'std::bad_function_call'
  what():  bad_function_call
```

**可能原因**：
- C++ 版本不兼容（项目用 C++17，但 Drogon 或库用了 C++20）
- std::function 类型转换问题
- 回调函数签名不匹配

**调试方法**：
```bash
# 使用 lldb 调试
lldb ./test/OAuth2Test_test
(lldb) b std::bad_function_call
(lldb) run
# 当异常抛出时
(lldb) bt
(lldb) thread list
(lldb) frame select 0
(lldb) variable list
```

### 2. 动态库加载错误

**症状**：
```
dyld: Library not loaded: @rpath/libdrogon.dylib
```

**解决方案**：
```bash
# 检查 Drogon 安装
ls -la /usr/local/lib/libdrogon*

# 更新动态库缓存
sudo update_dyld_shared_cache

# 检查链接
otool -L ./test/OAuth2Test_test
```

### 3. 数据库连接错误

**症状**：
```
Connection refused: localhost:5432
```

**解决方案**：
```bash
# 检查 PostgreSQL 状态
brew services list

# 重启 PostgreSQL
brew services restart postgresql@14

# 检查端口
netstat -an | grep 5432

# 手动测试连接
psql -h localhost -U test -d oauth_test
```

## 高级调试技巧

### 使用 Instruments

```bash
# 打开 Instruments 分析
instruments -t "System Trace" ./test/OAuth2Test_test

# 或使用 Allocations 工具
instruments -t "Allocations" ./test/OAuth2Test_test
```

### 使用 dtrace

```bash
# 追踪系统调用
sudo dtrace -n 'syscall:::entry { printf("%s", probefunc); }' -c "./test/OAuth2Test_test"

# 追踪函数调用
sudo dtrace -n 'pid$target:::function-entry { printf("%s", probefunc); }' -p $(pgrep OAuth2Test_test)
```

### 查看详细日志

```bash
# 设置 Drogon 日志级别
export LOG_LEVEL=debug

# 运行测试并捕获日志
./test/OAuth2Test_test 2>&1 | tee test_debug.log
```

## 与 Docker Linux 环境对比

| 特性 | Docker Linux | macOS 本地 |
|------|-------------|-----------|
| **环境隔离** | ✅ 完全隔离 | ⚠️ 本机环境 |
| **可重现性** | ✅ 高度一致 | ⚠️ 依赖本地配置 |
| **调试便利性** | ⚠️ 需要进入容器 | ✅ 可直接使用 lldb |
| **性能分析** | ⚠️ 受容器限制 | ✅ 可用 Instruments |
| **启动速度** | ⚠️ 较慢（首次 10-15 分钟） | ✅ 较快（已安装依赖） |

## 预期结果

### 场景 1: 测试成功

```
✅ SUCCESS: 所有测试通过！
macOS 环境测试完全正常
```

**下一步**：
1. 更新 CI 配置，重新启用测试
2. 更新文档说明 macOS 已修复

### 场景 2: std::bad_function_call 错误

```
❌ FAILED: 测试失败 (退出码: 134)

⚠ 发现 std::bad_function_call 错误
```

**下一步**：
1. 使用 lldb 定位具体错误函数
2. 检查 Drogon 框架版本兼容性
3. 尝试降级或升级 Drogon 版本
4. 向 Drogon 项目提交 issue

### 场景 3: 其他错误

根据错误日志分析，可能需要：
- 调整 CMake 配置
- 修复特定的兼容性问题
- 更新依赖库版本

## 文件说明

- **macos-quick-verify.sh** - 自动化验证脚本
- **docs/macos-debug-guide.md** - 本文档

## 参考资源

- [Drogon Framework 文档](https://drogon.docsforge.com/)
- [macOS CI 配置](../.github/workflows/ci-macos.yml)
- [C++17 macOS 兼容性](https://en.cppreference.com/w/cpp/17)
- [LLDB 调试器指南](https://lldb.llvm.org/use/map.html)

---

**最后更新**: 2026-04-22  
**状态**: 📝 待验证  
**优先级**: 中（不影响 Linux/Windows CI）
