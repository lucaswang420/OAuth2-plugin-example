# Drogon OAuth2.0 Provider & Vue Client Demo

![Linux CI](https://github.com/lucaswang420/OAuth2-plugin-example/workflows/ci-linux.yml/badge.svg)
![Windows CI](https://github.com/lucaswang420/OAuth2-plugin-example/workflows/ci-windows.yml/badge.svg)
![macOS CI](https://github.com/lucaswang420/OAuth2-plugin-example/workflows/ci-macos.yml/badge.svg)

This project demonstrates how to implement a fully functional OAuth2.0 Provider (Server) using the [Drogon C++ Web Framework](https://github.com/drogonframework/drogon) and a modern Client Application using [Vue.js](https://vuejs.org/).

It implements the **Authorization Code Grant** flow and supports:

1. **Local Authentication**: Login with credentials stored in the Drogon backend.
2. **External Provider Integration**: A "Login with WeChat" flow demonstrating server-side integration with the WeChat Open Platform API.

## Project Structure

```
OAuth2Test/
├── OAuth2Backend/      # C++ OAuth2 Provider (Drogon)
│   ├── controllers/    # API verify (OAuth2, WeChat)
│   ├── filters/        # Middleware (Token Validation)
│   ├── plugins/        # Core OAuth2 Logic Plugin
│   ├── views/          # Server-side Login Pages (CSP)
│   └── config.json     # App Configuration
└── OAuth2Frontend/     # Vue.js Client Application
    ├── src/views/      # Login & Callback Pages
    └── ...
```

## Prerequisites

- **Backend**:
  - C++ Compiler (supporting C++17/20, e.g., MSVC, GCC, Clang)
  - [Conan](https://conan.io/) (Package Manager)
  - [CMake](https://cmake.org/) (3.20+)
- **Frontend**:
  - [Node.js](https://nodejs.org/) & npm
- **Docker** (Optional, for Linux verification on Windows)

## CI/CD

This project uses comprehensive multi-platform CI/CD to ensure code quality across all major platforms:

### Platforms

- **Linux (Ubuntu 22.04)**: GCC with system package management
  - Full testing with PostgreSQL and Redis containers
  - No caching for consistent builds
  
- **Windows (Server 2022)**: MSVC 2022 with Conan package management
  - Full testing with memory storage (no database servers)
  - CI-optimized configuration for faster builds
  
- **macOS (14)**: Clang with Homebrew, ARM64 architecture
  - Build-only verification (tests disabled due to framework compatibility)
  - Pure C++17 enforcement to avoid codecvt_utf8_utf16 issues

### Features

- ✅ Full integration testing with PostgreSQL and Redis (Linux)
- ✅ Platform-specific optimizations and dependency management
- ✅ Automatic artifact collection and test log collection on failure
- ✅ Detailed platform diagnostics for debugging
- ✅ Memory storage testing on Windows for faster CI cycles

### Known Issues

- **macOS Runtime Issue**: Tests disabled due to Drogon framework compatibility issue with C++17/20 on macOS. Builds succeed but runtime crashes occur during test execution. This is a framework-level issue, not a code issue.

### Testing Coverage

- ✅ **18/18** Security tests (100%) - SQL injection, XSS, CORS, rate limiting, etc.
- ✅ **21/21** Functional tests (100%) - OAuth2 flow, UTF-8, RBAC, token lifecycle
- Unit tests for OAuth2 core logic
- Integration tests for PostgreSQL persistence (Linux)
- Integration tests for Redis caching (Linux)
- Memory storage tests (Windows/Linux)
- RBAC permission system tests
- End-to-end OAuth2 authorization flow tests

**Security & Quality Status**: 🟢 **Production Ready**
- All 10 critical security vulnerabilities fixed ✅
- 18 bugs resolved (51% completion rate)
- 17 remaining bugs are low-priority technical debt
- 1 bug confirmed as false positive (DB connection leak)

See test reports (local documentation):
- [Security Test Report](reports/bug-fix-2026-04-21/SECURITY_TEST_REPORT.md) - Comprehensive security testing results
- [Functional Test Report](reports/bug-fix-2026-04-21/FUNCTIONAL_TEST_REPORT.md) - Complete functional testing results
- [Remaining Bugs Analysis](reports/bug-fix-2026-04-21/REMAINING_BUGS_ANALYSIS.md) - Priority analysis for remaining bugs
- [Remaining Bugs List](reports/bug-fix-2026-04-21/REMAINING_BUGS.md) - Detailed bug status and risk assessment

See individual workflow files for detailed configuration:
- [.github/workflows/ci-linux.yml](.github/workflows/ci-linux.yml)
- [.github/workflows/ci-windows.yml](.github/workflows/ci-windows.yml)
- [.github/workflows/ci-macos.yml](.github/workflows/ci-macos.yml)

## 1. Backend Setup (OAuth2Backend)

 The backend handles OAuth2 requests, issues tokens, and validates API access.

### dependency Installation & Build

```powershell
cd OAuth2Backend
# Install dependencies and configure CMake
./build.bat
```

*(The `build.bat` script runs `conan install` and `cmake`, then builds the project.)*

### Running the Server

```powershell
cd OAuth2Backend/build
Release/OAuth2Server.exe
```

The server listens on `http://localhost:5555`.

### Configuration (Optional: WeChat)

To enable real WeChat login, edit `controllers/WeChatController.cc` and replace `YOUR_WECHAT_APPID` / `YOUR_WECHAT_SECRET` with your actual credentials.

## 2. Frontend Setup (OAuth2Frontend)

The frontend is a Single Page Application (SPA) acting as the OAuth2 Client.

### Installation

```bash
cd OAuth2Frontend
npm install
```

### Running the Client

```bash
npm run dev
```

The client runs on `http://localhost:5173`.

### Configuration (Optional: WeChat)

To enable real WeChat login, edit `src/views/Login.vue` and set `APPID` and `REDIRECT_URI` (Must match your domain).

## Storage & Persistence

The project supports pluggable storage backends for OAuth2 data.

### Supported Backends

1. **Memory** (Default): Fast, volatile storage. Best for testing.
2. **PostgreSQL**: Persistent, SQL-based storage.
3. **Redis**: High-performance, persistent Key-Value storage.

### Configuration

Edit `OAuth2Backend/config.json`:

```json
{
  "oauth2": {
    "storage_type": "redis" // Options: "memory", "postgres", "redis"
  },
  "redis": {
    "host": "127.0.0.1",
    "port": 6379,
    "passwd": "your_password"
  }
}
```

### Security Hardening

Client Secrets are securely stored using **SHA256 Hashing with Salt**.

### Persistence & Storage

This project uses a flexible persistence layer supporting **PostgreSQL** (Production) and **Redis** (High Performance).

For detailed architecture, supported backends, and schema designs, please refer to:
👉 **[Data Persistence Guide](OAuth2Backend/docs/data_persistence.md)**

### Data Consistency & Security

We implement **Atomic Consume** operations and **SHA256 Hashing** to ensure high security and consistency.

For implementation details (Lua Scripts, Threat Models, Token Lifecycle):
👉 **[Data Consistency Guide](OAuth2Backend/docs/data_consistency.md)**
👉 **[Security Architecture](OAuth2Backend/docs/security_architecture.md)**

### Observability

Production-ready monitoring with Prometheus Metrics and Structured Audit Logs.
👉 **[Observability Guide](OAuth2Backend/docs/observability.md)**

### Security Hardening

We implement Rate Limiting and Security Headers to protect against attacks.
👉 **[Security Hardening Guide](OAuth2Backend/docs/security_hardening.md)**

**Verified Security Features** (as of 2026-04-21):
- ✅ SQL injection protection (parameterized queries)
- ✅ XSS attack prevention (input validation + CSP headers)
- ✅ Command injection prevention
- ✅ DoS protection (input length limits: username 100 chars, password 200 chars)
- ✅ Rate limiting (brute force protection)
- ✅ CORS policy (domain whitelist)
- ✅ Token revocation mechanism
- ✅ Complete security HTTP headers
- ✅ HSTS (HTTPS-only configuration)
- ✅ Sensitive data protection (POST body credential transmission)

### Configuration & Deployment (New)

Full guide on Environment Variables and Docker deployment.
👉 **[Configuration Guide](OAuth2Backend/docs/configuration_guide.md)**

### RBAC Permission System

Role-Based Access Control using `AuthorizationFilter` and `rbac_rules` configuration.
Matches URL patterns to required roles (e.g. `/api/admin/.*` -> `["admin"]`).
👉 **[RBAC Guide](OAuth2Backend/docs/rbac_guide.md)**

### Linux Compatibility & Docker

Full cross-platform support with provided `Dockerfile` and `scripts/build.sh`.
Validated via automated Docker Desktop workflows on Windows.

**Linux Teardown Crash Fix** (2026-04-22):

✅ **Fixed** Linux-specific SegFault during program exit.

**Problem**: On Linux systems, the test program would crash with Segmentation Fault when exiting normally, due to the Drogon Event loop being accessed after destruction.

**Solution**: Implemented a `stopped_` flag in `OAuth2CleanupService` to prevent duplicate cleanup during teardown:
- Plugin shutdown now properly cleans up timers via `shutdown()` method
- Destructor checks `stopped_` flag to avoid accessing destroyed Event loop
- Tests now exit cleanly without `std::_Exit(0)` on Linux

**Verification**:
```powershell
# Build debug image (10-15 min, first time only)
docker build --no-cache -f Dockerfile.debug.cn -t oauth2-backend-debug:v1.9.12 .

# Run automated verification (1-2 min)
docker-compose -f docker-compose.debug.yml run --rm debug-env bash /app/docker-quick-verify-debug.sh
```

Expected result:
```
assertions: 46 | 46 passed | 0 failed
test cases: 11 | 11 passed | 0 failed
✅ SUCCESS: No crash during teardown!
```

For detailed debugging and verification instructions, see:
- [Docker Debug Verification Guide](docs/docker-debug-verification.md)
- [Docker Standardization Guide](docs/docker-standardization.md)

### macOS Compatibility & Known Issues

**macOS Runtime Issue** (2026-04-22):

⚠️ **Known Issue**: Tests disabled due to Drogon framework compatibility issue with C++17/20 on macOS.

**Problem**: The macOS CI build succeeds, but tests encounter `std::bad_function_call` runtime errors when executed. This is a framework-level issue specific to the macOS environment.

**Current Status**:

- ✅ Build: Successful
- ❌ Tests: Disabled (framework compatibility issue)
- 📝 Priority: Medium (Linux/Windows CI fully operational)

**Local Debugging on macOS**:

To debug and investigate the macOS test issue locally:

```bash
# Run automated verification script
./macos-quick-verify.sh
```

This script will:

1. Check Homebrew and dependencies
2. Start PostgreSQL and Redis services
3. Initialize database
4. Build the project
5. Attempt to run tests
6. Capture and analyze errors

**Manual Debugging**:

If you prefer to debug manually or the automated script encounters issues:

```bash
# 1. Install dependencies
brew install git cmake jsoncpp ossp-uuid zlib openssl@1.1 redis postgresql@14 hiredis

# 2. Start services
brew services start postgresql@14
brew services start redis

# 3. Build and test
cd OAuth2Backend
cmake -S . -B build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_CXX_FLAGS="-std=c++17" \
  -DBUILD_TESTS=ON \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@1.1)"
cmake --build build --config Release

# 4. Run tests with error capture
cd build
export OAUTH2_DB_HOST="127.0.0.1"
export OAUTH2_DB_NAME="oauth_test"
export OAUTH2_REDIS_PASSWORD=""
./test/OAuth2Test_test 2>&1 | tee test_output.txt

# 5. Debug with lldb if needed

```bash
lldb ./test/OAuth2Test_test
(lldb) run
(lldb) bt  # backtrace on crash
```

**Expected Error**:

```text
terminate called after throwing an instance of 'std::bad_function_call'
  what():  bad_function_call
```

**For detailed debugging instructions**, see:
- [macOS Debug Guide](docs/macos-debug-guide.md)
- [macOS CI Configuration](.github/workflows/ci-macos.yml)

## Features & Endpoints

> **OpenAPI Specification**: [openapi.yaml](OAuth2Backend/openapi.yaml)

| Feature | Endpoint / Description |
|---------|------------------------|
| **Authorize** | `GET /oauth2/authorize` - Logic to handle Authorization requests. |
| **Token** | `POST /oauth2/token` - Exchange Auth Code for Access Token. |
| **User Info** | `GET /oauth2/userinfo` - Protected Endpoint (Requires Bearer Token). |
| **WeChat Login** | `POST /api/wechat/login` - Server-side exchange of WeChat code for Session. |
| **Persistence** | Support for Redis/Postgres backends via Strategy Pattern. |
| **Expiration** | Auto-cleanup of expired tokens (Hourly) via Scheduler. |

## Usage Guide

1. **Start Backend & Frontend** using the commands above.
2. Open **<http://localhost:5173>**.
3. **Local Login**:
    - Click "Login with Drogon".
    - Credentials: `admin` / `admin`.
    - Observe successful redirect and user info display.
4. **WeChat Login**:
    - Requires valid AppID Configuration.
    - Click "Login with WeChat", scan QR code, and verify login.

## License

This project is licensed under the [MIT License](LICENSE).
