#!/bin/bash
# rebuild-debug-image.sh - Rebuild the OAuth2 debug Docker image

set -e

# Load common environment
source "$(dirname "$0")/env_common.sh"

echo "========================================"
echo "Rebuilding OAuth2 Backend Dev Image"
echo "========================================"
echo ""
echo "This will take a few minutes..."
echo ""

# Build the image using the unified Dockerfile and the backend-dev target
cd "$PROJECT_DIR"
docker build --no-cache -f Dockerfile --target backend-dev -t oauth2-backend-dev:latest .

echo ""
echo "========================================"
echo "Build completed!"
echo "========================================"
echo ""
echo "Verifying installation..."
docker run --rm oauth2-backend-dev:latest bash -c "
  echo 'Checking Drogon files:'
  # Note: Drogon is installed in /usr/local in the Dockerfile
  ls -la /usr/local/include/drogon/drogon.h && echo '  [PASS] Headers found'
  echo ''
  echo 'Drogon dev environment ready!'
"
