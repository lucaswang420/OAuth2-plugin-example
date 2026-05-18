# Docker 容器和镜像规范指南

本文档规定了本项目在 Docker 环境下的命名规范、构建流程及自动化验证方法。本项目采用统一的多阶段构建 `Dockerfile`。

## 1. 规范化命名 (Standardized Naming)

### 1.1 镜像命名规范

| 镜像用途 | 镜像名称 | 标签 | 构建目标 (--target) | 说明 |
|---------|---------|------|--------------------|---------|
| 生产后端 | `oauth2-backend` | `v1.9.12` | `backend-runtime` | 仅包含运行时，体积小 |
| 调试后端 | `oauth2-backend-debug` | `v1.9.12` | `backend-dev` | 包含完整编译工具链 |
| 生产前端 | `oauth2-frontend` | `latest` | `frontend-runtime` | Nginx + 静态资源 |

### 1.2 容器命名规范

格式：`oauth2-{service}[-debug]`

| 服务类型 | 容器名称 (Release) | 容器名称 (Debug) |
|-----|-------------------|-----------------|
| 后端服务 | `oauth2-backend` | `oauth2-backend-debug` |
| 前端服务 | `oauth2-frontend` | - |
| 数据库 (PostgreSQL) | `oauth2-postgres` | `oauth2-postgres-debug` |
| 缓存 (Redis) | `oauth2-redis` | `oauth2-redis-debug` |

### 1.3 网络命名规范
*   **Release 网络**: `oauth2-net`
*   **Debug 网络**: `oauth2-debug-net`

---

## 2. 构建与部署流程 (Build & Deployment)

### 2.1 环境清理
在重新构建前，建议清理旧的容器和未使用的镜像：
```powershell
# 使用脚本一键清理
.\scripts\cleanup-docker.sh

# 或手动清理
docker-compose -f docker-compose.yml down --remove-orphans
docker-compose -f docker-compose.debug.yml down --remove-orphans
docker image prune -f
```

### 2.2 调试环境 (Debug)
用于开发测试，支持挂载源码、GDB 调试和单元测试。

*   **构建镜像**:
    ```powershell
    # 使用统一 Dockerfile 的 backend-dev 目标构建
    docker build --target backend-dev -t oauth2-backend-debug:v1.9.12 .
    ```
*   **启动服务**:
    ```powershell
    docker-compose -f docker-compose.debug.yml up -d
    ```
*   **进入容器**:
    ```powershell
    docker-compose -f docker-compose.debug.yml run --rm debug-env bash
    ```

### 2.3 生产环境 (Release)
用于模拟真实部署，镜像体积小，安全性高。

*   **构建镜像**:
    ```powershell
    # 后端镜像
    docker build --target backend-runtime -t oauth2-backend:v1.9.12 .
    # 前端镜像
    docker build --target frontend-runtime -t oauth2-frontend:latest .
    ```
*   **启动服务**:
    ```powershell
    docker-compose -f docker-compose.yml up -d
    ```

---

## 3. 自动化验证 (Automated Verification)

### 3.1 调试环境验证 (`docker-quick-verify-debug.sh`)
该脚本在容器内部运行，涵盖从依赖检查到编译测试的全流程。

*   **运行命令 (已针对 Windows 路径/换行符优化)**:
    ```powershell
    docker-compose -f docker-compose.debug.yml run --rm debug-env bash -c "find scripts -name '*.sh' -exec sed -i 's/\r//' {} + && tr -d '\r' < /app/docker-quick-verify-debug.sh > /tmp/v.sh && bash /tmp/v.sh"
    ```
*   **验证步骤**:
    1. 检查 Drogon 框架是否存在。
    2. 等待 PostgreSQL 和 Redis 就绪。
    3. 执行数据库初始化。
    4. 并行编译项目 (`cmake --build . --parallel $(nproc)`)。
    5. 运行所有单元测试。

### 3.2 生产环境验证 (`docker-quick-verify-release.sh`)
该脚本在宿主机运行，验证服务的外部健康状态。

*   **运行命令**:
    ```powershell
    # Windows PowerShell
    .\scripts\docker-quick-verify-release.sh
    # Bash/WSL
    bash scripts/docker-quick-verify-release.sh
    ```
*   **验证步骤**:
    1. 检查所有容器运行状态。
    2. 验证 HTTP 接口响应 (Health Check / OAuth2 Endpoints)。
    3. 扫描后端日志中的异常报错。

---

## 4. 故障排查 (Troubleshooting)

*   **换行符报错**: 如果遇到 `$'\r': command not found`，请确保脚本以 `LF` 格式保存，或使用上述 `sed` 转换命令。
*   **路径转换问题**: Git Bash 用户如遇挂载路径报错，请在命令前添加 `MSYS_NO_PATHCONV=1`。
*   **构建性能**: 建议分配 CPU 4+，内存 4GB+。可以使用 `DOCKER_BUILDKIT=1` 加速构建。

---
**最后更新**: 2026-05-18  
**版本**: 2.1.0
