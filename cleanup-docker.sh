#!/bin/bash
# Cleanup script for OAuth2 project Docker resources
# Removes all project-related containers, images, and dangling images

set -e

echo "========================================"
echo "OAuth2 Project Docker Cleanup"
echo "========================================"
echo ""

# Stop and remove all project-related containers
echo "[1/5] Stopping containers..."
docker-compose -f docker-compose.yml down --remove-orphans 2>/dev/null || true
docker-compose -f docker-compose.debug.yml down --remove-orphans 2>/dev/null || true

# Stop specific containers by name
docker stop oauth2-postgres-debug oauth2-redis-debug oauth2-backend-debug 2>/dev/null || true
docker stop oauth2-postgres-release oauth2-redis-release oauth2-backend-release 2>/dev/null || true
docker stop oauth2-frontend-release 2>/dev/null || true

echo "✓ Containers stopped"

# Remove project containers
echo ""
echo "[2/5] Removing containers..."
docker rm oauth2-postgres-debug oauth2-redis-debug oauth2-backend-debug 2>/dev/null || true
docker rm oauth2-postgres-release oauth2-redis-release oauth2-backend-release 2>/dev/null || true
docker rm oauth2-frontend-release 2>/dev/null || true

echo "✓ Containers removed"

# Remove project images
echo ""
echo "[3/5] Removing project images..."
docker rmi oauth2-backend-release:v1.9.12 2>/dev/null || true
docker rmi oauth2-backend-debug:v1.9.12 2>/dev/null || true
docker rmi oauth2-frontend-release:latest 2>/dev/null || true

# Note: Old image names (oauth2-server:latest, oauth2-client:latest) have been replaced
# with oauth2-backend-release:v1.9.12 and oauth2-frontend-release:latest

echo "✓ Images removed"

# Clean up dangling images
echo ""
echo "[4/5] Cleaning dangling images..."
docker image prune -f

echo "✓ Dangling images removed"

# Clean up build cache volume
echo ""
echo "[5/5] Cleaning build cache..."
docker volume rm oauth2-build-cache 2>/dev/null || echo "  No build cache to remove"

echo "✓ Build cache cleaned"

echo ""
echo "========================================"
echo "Cleanup completed!"
echo "========================================"
echo ""
echo "Next steps:"
echo "  1. Build debug image: docker build -f Dockerfile.debug.cn -t oauth2-backend-debug:v1.9.12 ."
echo "  2. Run verification: docker-compose -f docker-compose.debug.yml run --rm debug-env bash /app/docker-quick-verify-debug.sh"
