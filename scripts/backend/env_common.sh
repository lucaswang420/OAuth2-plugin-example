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
