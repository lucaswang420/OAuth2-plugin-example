#!/bin/bash
# Quick verification script for release Docker environment
# This script validates the release environment is working correctly

set -e

echo "========================================"
echo "Release Environment Quick Verification"
echo "========================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BACKEND_CONTAINER="oauth2-backend-release"
FRONTEND_CONTAINER="oauth2-frontend-release"
POSTGRES_CONTAINER="oauth2-postgres-release"
REDIS_CONTAINER="oauth2-redis-release"
BACKEND_URL="http://localhost:5555"
FRONTEND_URL="http://localhost:8080"

# Step 1: Check if containers are running
echo "[1/6] Checking container status..."
echo "-------------------------------------------"

ALL_RUNNING=true

for container in $BACKEND_CONTAINER $FRONTEND_CONTAINER $POSTGRES_CONTAINER $REDIS_CONTAINER; do
  if docker ps --filter "name=$container" --format "{{.Names}}" | grep -q "^${container}$"; then
    echo "✓ $container is running"
  else
    echo "✗ $container is NOT running"
    ALL_RUNNING=false
  fi
done

if [ "$ALL_RUNNING" = false ]; then
  echo ""
  echo "❌ ERROR: Some containers are not running!"
  echo "Please start the release environment:"
  echo "  docker-compose -f docker-compose.yml up -d"
  exit 1
fi

# Step 2: Wait for databases to be ready
echo ""
echo "[2/6] Waiting for databases..."
echo "-------------------------------------------"

# Wait for PostgreSQL
for i in {1..30}; do
  if docker exec $POSTGRES_CONTAINER pg_isready -U test >/dev/null 2>&1; then
    echo "✓ PostgreSQL is ready"
    break
  fi
  if [ $i -eq 30 ]; then
    echo "✗ ERROR: PostgreSQL not ready after 30 seconds"
    exit 1
  fi
  sleep 1
done

# Wait for Redis
for i in {1..30}; do
  if docker exec $REDIS_CONTAINER redis-cli -a redis_secret_pass ping >/dev/null 2>&1; then
    echo "✓ Redis is ready"
    break
  fi
  if [ $i -eq 30 ]; then
    echo "✗ ERROR: Redis not ready after 30 seconds"
    exit 1
  fi
  sleep 1
done

# Step 3: Verify database initialization
echo ""
echo "[3/6] Verifying database initialization..."
echo "-------------------------------------------"

export PGPASSWORD=postgres_secret_pass
if docker exec $POSTGRES_CONTAINER psql -U test -d oauth2_db -c "SELECT 1 FROM oauth2_clients LIMIT 1;" >/dev/null 2>&1; then
  echo "✓ Database tables exist"
else
  echo "✗ ERROR: Database tables not found!"
  echo "Checking database..."
  docker exec $POSTGRES_CONTAINER psql -U test -d oauth2_db -c "\dt" || true
  exit 1
fi

# Verify required tables
REQUIRED_TABLES=("oauth2_clients" "oauth2_codes" "oauth2_access_tokens" "oauth2_refresh_tokens" "users" "roles")
MISSING_TABLES=0

for table in "${REQUIRED_TABLES[@]}"; do
  if docker exec $POSTGRES_CONTAINER psql -U test -d oauth2_db -c "SELECT 1 FROM $table LIMIT 1;" >/dev/null 2>&1; then
    echo "  ✓ Table '$table' exists"
  else
    echo "  ✗ Table '$table' MISSING"
    MISSING_TABLES=$((MISSING_TABLES + 1))
  fi
done

if [ $MISSING_TABLES -gt 0 ]; then
  echo ""
  echo "❌ ERROR: $MISSING_TABLES required table(s) missing!"
  exit 1
fi

# Step 4: Test backend HTTP endpoints
echo ""
echo "[4/6] Testing backend HTTP endpoints..."
echo "-------------------------------------------"

# Test health endpoint
if curl -sf "$BACKEND_URL/health" >/dev/null 2>&1; then
  echo "✓ Health endpoint is responding"
else
  echo "✗ ERROR: Health endpoint not responding!"
  echo "URL: $BACKEND_URL/health"
  echo "Backend logs:"
  docker logs --tail 20 $BACKEND_CONTAINER
  exit 1
fi

# Test metrics endpoint
if curl -sf "$BACKEND_URL/metrics" >/dev/null 2>&1; then
  echo "✓ Metrics endpoint is responding"
else
  echo "⚠ WARNING: Metrics endpoint not responding"
fi

# Step 4.5: Basic OAuth2 integration tests
echo ""
echo "[4.5/6] Running OAuth2 integration tests..."
echo "-------------------------------------------"

# Test 1: Login endpoint (POST /oauth2/login)
LOGIN_RESPONSE=$(curl -s -X POST "$BACKEND_URL/oauth2/login" \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin","client_id":"vue-client","redirect_uri":"http://localhost:8080/callback"}')

if echo "$LOGIN_RESPONSE" | grep -q "redirect_uri"; then
  echo "✓ Login endpoint is working"
else
  echo "⚠ WARNING: Login endpoint returned unexpected response"
  echo "Response: $LOGIN_RESPONSE"
fi

# Test 2: Token endpoint (POST /oauth2/token)
# Note: This will fail without a valid authorization code, but we check if endpoint responds
TOKEN_RESPONSE=$(curl -s -X POST "$BACKEND_URL/oauth2/token" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=client_credentials&client_id=vue-client&client_secret=123456")

HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BACKEND_URL/oauth2/token" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=client_credentials&client_id=vue-client&client_secret=123456")

if [ "$HTTP_CODE" != "000" ]; then
  echo "✓ Token endpoint is responding (HTTP $HTTP_CODE)"
else
  echo "✗ ERROR: Token endpoint not responding!"
  exit 1
fi

# Test 3: Protected API endpoint (should fail without auth, but endpoint exists)
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BACKEND_URL/oauth2/userinfo")

if [ "$HTTP_CODE" = "401" ] || [ "$HTTP_CODE" = "403" ]; then
  echo "✓ Protected endpoint requires authentication (HTTP $HTTP_CODE) ✓"
elif [ "$HTTP_CODE" != "000" ]; then
  echo "✓ Protected endpoint is responding (HTTP $HTTP_CODE)"
else
  echo "⚠ WARNING: Protected endpoint not responding"
fi

# Test OAuth2 authorize endpoint (should return error, but endpoint exists)
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BACKEND_URL/oauth2/authorize?response_type=code&client_id=vue-client&redirect_uri=http://localhost:8080/callback")
if [ "$HTTP_CODE" != "000" ]; then
  echo "✓ OAuth2 authorize endpoint is responding (HTTP $HTTP_CODE)"
else
  echo "✗ ERROR: OAuth2 authorize endpoint not responding!"
  exit 1
fi

# Step 5: Test frontend
echo ""
echo "[5/6] Testing frontend..."
echo "-------------------------------------------"

if curl -sf "$FRONTEND_URL" >/dev/null 2>&1; then
  echo "✓ Frontend is responding"
else
  echo "⚠ WARNING: Frontend not responding (may be starting)"
fi

# Step 6: Check logs for errors
echo ""
echo "[6/6] Checking for errors in logs..."
echo "-------------------------------------------"

# Check backend logs for common errors
if docker logs $BACKEND_CONTAINER 2>&1 | grep -i "error\|exception\|fatal" | grep -v "level=error" | head -5; then
  echo ""
  echo "⚠ WARNING: Found errors in backend logs"
  echo "Recent logs:"
  docker logs --tail 50 $BACKEND_CONTAINER
else
  echo "✓ No critical errors found in backend logs"
fi

# Summary
echo ""
echo "========================================"
echo "✅ SUCCESS: Release environment is healthy!"
echo "========================================"
echo ""
echo "Services running:"
echo "  - Backend:   $BACKEND_URL"
echo "  - Frontend:  $FRONTEND_URL"
echo "  - PostgreSQL: localhost:5433"
echo "  - Redis:      localhost:6380"
echo "  - Prometheus: http://localhost:9090"
echo ""
echo "Quick test commands:"
echo "  # Test health endpoint"
echo "  curl $BACKEND_URL/health"
echo ""
echo "  # Test metrics endpoint"
echo "  curl $BACKEND_URL/metrics"
echo ""
echo "  # View backend logs"
echo "  docker-compose -f docker-compose.yml logs -f oauth2-backend"
echo ""
echo "  # Enter backend container"
echo "  docker exec -it $BACKEND_CONTAINER bash"
echo "========================================"
