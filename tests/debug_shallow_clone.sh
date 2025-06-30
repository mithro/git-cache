#!/bin/bash

# Debug script for shallow clone test

set -x  # Enable debug output

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_DIR/git-cache"

# Set GIT_CACHE environment variable
export GIT_CACHE="$PROJECT_DIR/.cache/git"

# Clean up first
echo "Cleaning up test repositories..."
cd "$PROJECT_DIR" && make clean-cache >/dev/null 2>&1 || {
    rm -rf "$GIT_CACHE" 2>/dev/null || true
    rm -rf "$PROJECT_DIR/github" 2>/dev/null || true
}

echo "Running shallow clone test..."
echo "Command: $BINARY clone --strategy shallow --depth 1 https://github.com/octocat/Hello-World.git"

# Run with full output
"$BINARY" clone --strategy shallow --depth 1 https://github.com/octocat/Hello-World.git
EXIT_CODE=$?

echo "Exit code: $EXIT_CODE"

# Check what was created
echo "Checking created directories..."
ls -la "$PROJECT_DIR/.cache/git/github.com/" 2>&1 || echo "Cache directory not found"
ls -la "$PROJECT_DIR/github/octocat/" 2>&1 || echo "Checkout directory not found"
ls -la "$PROJECT_DIR/github/mithro/" 2>&1 || echo "Modifiable directory not found"

exit $EXIT_CODE