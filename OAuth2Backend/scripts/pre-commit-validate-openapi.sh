#!/bin/bash
# Pre-commit hook to validate OpenAPI specification
# This script ensures that OpenAPI documentation is valid before committing

set -e

echo "🔍 Validating OpenAPI specification..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Find the project root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Check if OpenAPI spec can be generated
echo "Building project to generate OpenAPI spec..."
cd "$PROJECT_ROOT/build"

# Run the OpenAPI validation tests
echo "Running OpenAPI validation tests..."
if ctest -C Release -R OpenApiGenerator --output-on-failure; then
    echo -e "${GREEN}✓ OpenAPI validation tests passed${NC}"
else
    echo -e "${RED}✗ OpenAPI validation tests failed${NC}"
    echo "Please fix the OpenAPI documentation before committing."
    exit 1
fi

# Check if openapi.json file exists and is valid JSON
OPENAPI_FILE="$PROJECT_ROOT/build/Release/docs/api/openapi.json"
if [ -f "$OPENAPI_FILE" ]; then
    echo "Validating OpenAPI JSON structure..."
    if python3 -m json.tool "$OPENAPI_FILE" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ OpenAPI JSON is valid${NC}"
    else
        echo -e "${RED}✗ OpenAPI JSON is invalid${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}⚠ OpenAPI file not found at $OPENAPI_FILE${NC}"
    echo "This is expected on first commit or if build hasn't been run."
fi

# Check for required OpenAPI fields
echo "Checking required OpenAPI fields..."
if [ -f "$OPENAPI_FILE" ]; then
    REQUIRED_FIELDS=("openapi" "info" "paths" "servers")
    for field in "${REQUIRED_FIELDS[@]}"; do
        if jq -e ".${field}" "$OPENAPI_FILE" > /dev/null 2>&1; then
            echo -e "${GREEN}✓ Field '${field}' exists${NC}"
        else
            echo -e "${RED}✗ Required field '${field}' is missing${NC}"
            exit 1
        fi
    done
fi

echo -e "${GREEN}✓ OpenAPI validation completed successfully${NC}"
exit 0
