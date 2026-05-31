#!/bin/bash
# env_common.sh - Common environment variables for Linux backend scripts

# Determine script and project directories
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_DIR="$( dirname "$( dirname "$SCRIPT_DIR" )" )"

# Validation
if [ ! -d "$PROJECT_DIR/OAuth2Plugin" ]; then
    echo "[Error] Project structure invalid. Could not find OAuth2Plugin at $PROJECT_DIR"
    exit 1
fi

export PROJECT_DIR
export SCRIPT_DIR

# Relocated Docker assets (repo-structure-refactor moved these out of the root
# into deploy/docker/). Scripts must reference them via these variables instead
# of bare `docker-compose` / `-f Dockerfile`, which assumed root-level files.
COMPOSE_FILE="$PROJECT_DIR/deploy/docker/docker-compose.yml"
DOCKERFILE="$PROJECT_DIR/deploy/docker/Dockerfile"
export COMPOSE_FILE
export DOCKERFILE
