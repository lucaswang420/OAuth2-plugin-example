#!/bin/bash

echo "================================"
echo "OAuth2 Plugin Docker 清理脚本"
echo "================================"
echo ""

# Step 1: 停止并删除所有容器
echo "📦 停止并删除所有容器..."
docker-compose -f ../docker-compose.yml down --remove-orphans
docker-compose -f ../docker-compose.debug.yml down --remove-orphans

# Step 2: 删除所有相关的 volumes
echo "🗑️  删除所有相关 volumes..."
docker volume rm oauth2-plugin_postgres_prod 2>/dev/null || echo "  - oauth2-plugin_postgres_prod (不存在或已删除)"
docker volume rm oauth2-plugin_redis_prod 2>/dev/null || echo "  - oauth2-plugin_redis_prod (不存在或已删除)"
docker volume rm oauth2-plugin_prometheus_prod 2>/dev/null || echo "  - oauth2-plugin_prometheus_prod (不存在或已删除)"
docker volume rm oauth2-plugin_build_cache 2>/dev/null || echo "  - oauth2-plugin_build_cache (不存在或已删除)"
docker volume rm oauth2-plugin_postgres_debug 2>/dev/null || echo "  - oauth2-plugin_postgres_debug (不存在或已删除)"
docker volume rm oauth2-plugin_redis_debug 2>/dev/null || echo "  - oauth2-plugin_redis_debug (不存在或已删除)"

# Step 3: 删除项目相关的镜像
echo "🖼️  删除项目相关镜像..."
docker image rm oauth2-backend-debug:v1.9.12 2>/dev/null || echo "  - oauth2-backend-debug:v1.9.12 (不存在或已删除)"
docker image rm oauth2-backend-frontend 2>/dev/null || echo "  - oauth2-backend-frontend (不存在或已删除)"

# Step 4: 清理悬空镜像和构建缓存
echo "🧹 清理悬空镜像和构建缓存..."
docker image prune -f
docker builder prune -f

echo ""
echo "✅ 清理完成！"
echo "📊 当前 Docker 状态:"
echo "  - 容器: $(docker ps -aq | wc -l) 个"
echo "  - 镜像: $(docker images | wc -l) 个"
echo "  - Volumes: $(docker volume ls | wc -l) 个"