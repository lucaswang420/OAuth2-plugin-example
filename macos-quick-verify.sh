#!/bin/bash
# macOS 本地测试环境验证脚本
# 用于排查 macOS CI 中的测试问题

set -e

echo "========================================"
echo "macOS 本地测试环境验证"
echo "========================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 配置
POSTGRES_VERSION="14"
REDIS_VERSION="default"
DB_NAME="oauth_test"
DB_USER="test"
DB_PASSWORD="123456"
BUILD_TYPE="Release"

# 步骤 1: 检查 Homebrew
echo "[1/8] 检查 Homebrew..."
echo "-------------------------------------------"
if ! command -v brew &> /dev/null; then
    echo -e "${RED}✗ ERROR: Homebrew 未安装${NC}"
    echo "请先安装 Homebrew: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi
echo -e "${GREEN}✓ Homebrew 已安装${NC}"
brew --version

# 步骤 2: 检查并安装依赖
echo ""
echo "[2/8] 检查依赖..."
echo "-------------------------------------------"

DEPENDENCIES=("git" "cmake" "jsoncpp" "ossp-uuid" "zlib" "openssl@1.1" "redis" "postgresql@${POSTGRES_VERSION}" "hiredis")
MISSING_DEPS=0

for dep in "${DEPENDENCIES[@]}"; do
    if brew list "$dep" &> /dev/null; then
        echo -e "  ${GREEN}✓${NC} $dep"
    else
        echo -e "  ${YELLOW}✗${NC} $dep (未安装)"
        MISSING_DEPS=$((MISSING_DEPS + 1))
    fi
done

if [ $MISSING_DEPS -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}发现 $MISSING_DEPS 个缺失的依赖${NC}"
    echo "正在安装..."
    brew install ${DEPENDENCIES[@]}
else
    echo -e "${GREEN}✓ 所有依赖已安装${NC}"
fi

# 步骤 3: 启动 PostgreSQL
echo ""
echo "[3/8] 启动 PostgreSQL..."
echo "-------------------------------------------"

if brew services list | grep postgresql@${POSTGRES_VERSION} | grep -q started; then
    echo -e "${GREEN}✓ PostgreSQL 已在运行${NC}"
else
    echo "启动 PostgreSQL 服务..."
    brew services start postgresql@${POSTGRES_VERSION}

    # 等待 PostgreSQL 就绪
    echo "等待 PostgreSQL 启动..."
    for i in {1..15}; do
        if pg_isready -h localhost -U $DB_USER &> /dev/null; then
            echo -e "${GREEN}✓ PostgreSQL 已就绪${NC}"
            break
        fi
        if [ $i -eq 15 ]; then
            echo -e "${RED}✗ ERROR: PostgreSQL 启动超时${NC}"
            exit 1
        fi
        sleep 2
    done
fi

# 步骤 4: 启动 Redis
echo ""
echo "[4/8] 启动 Redis..."
echo "-------------------------------------------"

if brew services list | grep redis | grep -q started; then
    echo -e "${GREEN}✓ Redis 已在运行${NC}"
else
    echo "启动 Redis 服务..."
    brew services start redis
    sleep 2

    if redis-cli -h localhost ping &> /dev/null; then
        echo -e "${GREEN}✓ Redis 已就绪${NC}"
    else
        echo -e "${RED}✗ ERROR: Redis 启动失败${NC}"
        exit 1
    fi
fi

# 步骤 5: 初始化数据库
echo ""
echo "[5/8] 初始化数据库..."
echo "-------------------------------------------"

export PGPASSWORD=$DB_PASSWORD

# 检查数据库是否存在
if psql -h localhost -U $DB_USER -d postgres -c "SELECT 1 FROM pg_database WHERE datname='$DB_NAME';" | grep -q 1; then
    echo "数据库 $DB_NAME 已存在"
else
    echo "创建数据库 $DB_NAME..."
    psql -h localhost -U $DB_USER -d postgres -c "CREATE DATABASE $DB_NAME;"
    psql -h localhost -U $DB_USER -d postgres -c "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';"
    psql -h localhost -U $DB_USER -d postgres -c "GRANT ALL PRIVILEGES ON DATABASE $DB_NAME TO $DB_USER;"
    echo -e "${GREEN}✓ 数据库创建成功${NC}"
fi

# 检查表是否存在
TABLE_COUNT=$(psql -h localhost -U $DB_USER -d $DB_NAME -c "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='public';" -t | tr -d ' ')

if [ "$TABLE_COUNT" -gt 0 ]; then
    echo "数据库已初始化 ($TABLE_COUNT 个表)"
else
    echo "初始化数据库表..."
    cd OAuth2Backend

    psql -h localhost -U $DB_USER -d $DB_NAME -f sql/001_oauth2_core.sql
    psql -h localhost -U $DB_USER -d $DB_NAME -f sql/002_users_table.sql
    psql -h localhost -U $DB_USER -d $DB_NAME -f sql/003_rbac_schema.sql

    echo -e "${GREEN}✓ 数据库初始化完成${NC}"
    cd ..
fi

# 步骤 6: 编译项目
echo ""
echo "[6/8] 编译项目..."
echo "-------------------------------------------"

cd OAuth2Backend

# 清理旧的构建
if [ -d "build" ]; then
    echo "清理旧的构建目录..."
    rm -rf build
fi

# 配置 CMake
echo "配置 CMake..."
OPENSSL_ROOT=$(brew --prefix openssl@1.1)
cmake -S . -B build -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_CXX_FLAGS="-std=c++17" \
    -DBUILD_TESTS=ON \
    -DOPENSSL_ROOT_DIR="$OPENSSL_ROOT" \
    -DCMAKE_FIND_FRAMEWORK=LAST

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ ERROR: CMake 配置失败${NC}"
    exit 1
fi

# 编译
echo "编译项目（这可能需要几分钟）..."
NPROC=$(sysctl -n hw.ncpu)
cmake --build build --config $BUILD_TYPE --parallel $NPROC

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ ERROR: 编译失败${NC}"
    exit 1
fi

echo -e "${GREEN}✓ 编译成功${NC}"

# 验证可执行文件
if [ ! -f "build/test/OAuth2Test_test" ] && [ ! -f "build/test/Debug/OAuth2Test_test" ]; then
    echo -e "${YELLOW}⚠ WARNING: 测试可执行文件未找到${NC}"
    echo "查找路径："
    find build -name "*Test*" -type f || true
else
    echo -e "${GREEN}✓ 测试可执行文件已找到${NC}"
fi

cd ..

# 步骤 7: 检查运行时环境
echo ""
echo "[7/8] 检查运行时环境..."
echo "-------------------------------------------"

# 检查 Dylib 路径
echo "检查 OpenSSL 路径..."
OPENSSL_PATH=$(brew --prefix openssl@1.1)
if [ -d "$OPENSSL_PATH/lib" ]; then
    echo "OpenSSL 库路径: $OPENSSL_PATH/lib"
    ls -la "$OPENSSL_PATH/lib/libssl"* || true
    ls -la "$OPENSSL_PATH/lib/libcrypto"* || true
fi

# 检查 Drogon 安装
echo "检查 Drogon 框架..."
if [ -f "/usr/local/lib/libdrogon.dylib" ]; then
    echo -e "${GREEN}✓ Drogon 库已安装${NC}"
    otool -L /usr/local/lib/libdrogon.dylib | head -10
else
    echo -e "${YELLOW}⚠ WARNING: Drogon 库未找到${NC}"
    echo "请检查 Drogon 是否正确安装"
fi

# 步骤 8: 尝试运行测试（如果存在）
echo ""
echo "[8/8] 尝试运行测试..."
echo "========================================"

cd OAuth2Backend/build

# 查找测试可执行文件
TEST_EXEC=""
if [ -f "test/OAuth2Test_test" ]; then
    TEST_EXEC="test/OAuth2Test_test"
elif [ -f "test/Debug/OAuth2Test_test" ]; then
    TEST_EXEC="test/Debug/OAuth2Test_test"
else
    echo -e "${YELLOW}⚠ WARNING: 未找到测试可执行文件${NC}"
    echo ""
    echo "build 目录内容："
    find . -name "*Test*" -type f || true
    echo ""
    echo "虽然测试文件未找到，但编译已成功完成！"
    echo "可以尝试手动查找测试可执行文件并运行。"
    exit 0
fi

echo "找到测试可执行文件: $TEST_EXEC"
echo ""
echo "设置环境变量..."
export OAUTH2_DB_HOST="127.0.0.1"
export OAUTH2_DB_NAME="$DB_NAME"
export OAUTH2_DB_PASSWORD="$DB_PASSWORD"
export OAUTH2_DB_USER="$DB_USER"
export OAUTH2_REDIS_HOST="127.0.0.1"
export OAUTH2_REDIS_PASSWORD=""

echo ""
echo "运行测试..."
echo "========================================"

# 运行测试并捕获输出
set +e  # 测试失败时继续执行，以便我们能看到错误

if ./$TEST_EXEC 2>&1 | tee test_output.txt; then
    EXIT_CODE=$?
    echo ""
    echo "========================================"
    if [ $EXIT_CODE -eq 0 ]; then
        echo -e "${GREEN}✅ SUCCESS: 所有测试通过！${NC}"
        echo "macOS 环境测试完全正常"
    else
        echo -e "${GREEN}✅ 测试通过，但有警告${NC}"
    fi
else
    EXIT_CODE=$?
    echo ""
    echo "========================================"
    echo -e "${RED}❌ FAILED: 测试失败 (退出码: $EXIT_CODE)${NC}"
    echo ""
    echo "错误分析："
    echo "----------------------------------------"

    # 检查常见的 macOS 错误
    if grep -q "std::bad_function_call" test_output.txt; then
        echo -e "${YELLOW}⚠ 发现 std::bad_function_call 错误${NC}"
        echo "这是 C++ stdlib 函数调用问题，通常由以下原因引起："
        echo "  1. C++ 版本不兼容"
        echo "  2. std::function 类型转换问题"
        echo "  3. Drogon 框架的 C++17/20 兼容性问题"
        echo ""
        echo "可能的解决方案："
        echo "  - 检查是否混合使用了不同的 C++ 标准"
        echo "  - 确保 Drogon 和项目都使用相同的 C++ 标准"
        echo "  - 查看具体的堆栈跟踪以定位问题函数"
    fi

    if grep -q "dyld: Library not loaded" test_output.txt; then
        echo -e "${YELLOW}⚠ 发现动态库加载错误${NC}"
        echo "缺少必需的动态库，检查："
        echo "  - OpenSSL 库路径"
        echo "  - Drogon 库路径"
        echo "  - CMake FIND_FRAMEWORK 设置"
    fi

    if grep -q "Segmentation fault" test_output.txt; then
        echo -e "${RED}⚠ 发现段错误${NC}"
        echo "程序崩溃，可能原因："
        echo "  - 空指针解引用"
        echo "  - 内存访问错误"
        echo "  - 已释放的内存访问"
        echo ""
        echo "建议使用 lldb 进行调试"
    fi

    echo ""
    echo "完整测试日志已保存到: OAuth2Backend/build/test_output.txt"
    echo "堆栈跟踪信息:"
    echo "----------------------------------------"
    grep -A 20 "FAILED" test_output.txt || true
fi

cd ../..

# 清理提示
echo ""
echo "========================================"
echo "测试完成"
echo "========================================"
echo ""
echo "环境信息："
echo "  macOS 版本: $(sw_vers)"
echo "  处理器: $(uname -m)"
echo "  CMake: $(cmake --version | head -1)"
echo "  编译器: $(clang --version | head -1)"
echo ""
echo "服务状态："
echo "  PostgreSQL: $(brew services list | grep postgresql | awk '{print $2}')"
echo "  Redis: $(brew services list | grep redis | awk '{print $2}')"
echo ""
echo "下一步建议："
echo "  1. 查看测试日志: cat OAuth2Backend/build/test_output.txt"
echo "  2. 使用 lldb 调试: lldb OAuth2Backend/build/$TEST_EXEC"
echo "  3. 查看服务日志: brew services list"
echo "  4. 重启服务: brew services restart postgresql@14 redis"
