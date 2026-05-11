#!/bin/bash
set -e

# Initialize variables
BUILD_TYPE=Release
USE_CONAN=false
BUILD_DROGON=false
DROGON_VERSION="v1.9.12"
DROGON_INSTALL_PREFIX="/usr/local"

# Parse arguments
for arg in "$@"; do
    case $arg in
        --conan)
            USE_CONAN=true
            shift
            ;;
        --build-drogon)
            BUILD_DROGON=true
            shift
            ;;
        --drogon-version=*)
            DROGON_VERSION="${arg#*=}"
            shift
            ;;
        --drogon-prefix=*)
            DROGON_INSTALL_PREFIX="${arg#*=}"
            shift
            ;;
        Debug|Release|RelWithDebInfo|MinSizeRel)
            BUILD_TYPE=$arg
            ;;
    esac
done

echo "=========================================="
echo "OAuth2Backend Build Script (Linux)"
echo "=========================================="
echo "Build Type:        $BUILD_TYPE"
echo "Use Conan:         $USE_CONAN"
echo "Build Drogon:      $BUILD_DROGON"
echo "Drogon Version:    $DROGON_VERSION"
echo "Drogon Prefix:     $DROGON_INSTALL_PREFIX"
echo "=========================================="

# Check dependencies
if ! command -v cmake &> /dev/null; then
    echo "Error: 'cmake' not found. Please install CMake."
    echo "  sudo apt-get install cmake"
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo "Error: 'make' not found. Please install build tools."
    echo "  sudo apt-get install build-essential"
    exit 1
fi

if [ "$USE_CONAN" = true ]; then
    if ! command -v conan &> /dev/null; then
        echo "Error: 'conan' not found. Please install Conan or use system libraries."
        echo "  pip install conan"
        exit 1
    fi
fi

# Determine project directory (parent of where this script is)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_DIR="$( dirname "$SCRIPT_DIR" )"
BUILD_DIR="$PROJECT_DIR/build"

echo "Project Dir: $PROJECT_DIR"
echo "Build Dir:  $BUILD_DIR"

# Option 1: Build Drogon from source (recommended for Linux)
if [ "$BUILD_DROGON" = true ]; then
    echo ""
    echo "=========================================="
    echo "Building Drogon from source"
    echo "=========================================="

    DROGON_DIR="$PROJECT_DIR/../drogon"

    # Clone Drogon if not exists
    if [ ! -d "$DROGON_DIR" ]; then
        echo "Cloning Drogon $DROGON_VERSION..."
        git clone --depth 1 --branch $DROGON_VERSION https://github.com/drogonframework/drogon "$DROGON_DIR"
        cd "$DROGON_DIR"
        git submodule update --init --recursive
    else
        echo "Drogon directory already exists: $DROGON_DIR"
    fi

    # Build Drogon
    echo "Building Drogon..."
    mkdir -p "$DROGON_DIR/build"
    cd "$DROGON_DIR/build"

    cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=$DROGON_INSTALL_PREFIX \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_MYSQL=OFF

    make -j$(nproc)

    echo "Installing Drogon to $DROGON_INSTALL_PREFIX (requires sudo)..."
    sudo make install

    echo "Drogon build and installation complete!"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTS=ON"

if [ "$USE_CONAN" = true ]; then
    # Option 2: Use Conan (not recommended for Linux)
    echo ""
    echo "=========================================="
    echo "Using Conan for dependencies"
    echo "=========================================="

    # Detecting profile if not exists (fail-safe)
    if ! conan profile path default &> /dev/null; then
        echo "Conan profile 'default' not found. Detecting..."
        conan profile detect
    fi

    echo "Installing Conan dependencies..."
    conan install "$PROJECT_DIR" --build=missing -s build_type=$BUILD_TYPE

    # Configure CMake with Conan toolchain
    TOOLCHAIN_FILE="$BUILD_DIR/conan_toolchain.cmake"
    if [ -f "$TOOLCHAIN_FILE" ]; then
        echo "Using Conan toolchain: $TOOLCHAIN_FILE"
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE"
    fi
else
    # Option 3: Use system libraries (recommended for Linux)
    echo ""
    echo "=========================================="
    echo "Using system libraries"
    echo "=========================================="

    # Add Drogon prefix if custom build
    if [ "$BUILD_DROGON" = true ] || [ -d "$DROGON_INSTALL_PREFIX" ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_PREFIX_PATH=$DROGON_INSTALL_PREFIX"
        echo "Using Drogon from: $DROGON_INSTALL_PREFIX"
    fi

    echo "Note: Make sure system dependencies are installed:"
    echo "  sudo apt-get install -y \\"
    echo "    libjsoncpp-dev uuid-dev libpq-dev \\"
    echo "    libssl-dev zlib1g-dev libhiredis-dev"
fi

# Configure CMake
echo ""
echo "=========================================="
echo "Configuring CMake"
echo "=========================================="
echo "CMake Args: $CMAKE_ARGS"
cmake "$PROJECT_DIR" $CMAKE_ARGS

# Build
echo ""
echo "=========================================="
echo "Building OAuth2Backend"
echo "=========================================="
cmake --build . --config $BUILD_TYPE -- -j$(nproc)

echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo "Executable: $BUILD_DIR/OAuth2Server"
echo ""
echo "To run the server:"
echo "  cd $BUILD_DIR"
echo "  ./OAuth2Server"
echo ""
echo "To run tests:"
echo "  cd $BUILD_DIR"
echo "  ctest --output-on-failure"
echo "=========================================="
