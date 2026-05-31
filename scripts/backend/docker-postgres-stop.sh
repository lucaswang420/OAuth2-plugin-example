#!/usr/bin/env bash
# docker-postgres-stop.sh - Stop PostgreSQL in Docker (Linux/macOS)
set -euo pipefail

source "$(dirname "$0")/env_common.sh"

echo "Stopping PostgreSQL in Docker..."

cd "$PROJECT_DIR"
docker-compose -f "$COMPOSE_FILE" down

echo "[SUCCESS] PostgreSQL stopped"
echo ""
echo "To remove data volumes as well, run:"
echo "  docker-compose -f deploy/docker/docker-compose.yml down -v"
