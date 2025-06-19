#!/bin/bash

# Git cache integration test runner

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_DIR/git-cache"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local expected_exit_code="$2"
    shift 2
    local cmd="$@"
    
    echo -n "Running test: $test_name... "
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if eval "$cmd" >/dev/null 2>&1; then
        actual_exit_code=0
    else
        actual_exit_code=$?
    fi
    
    if [ "$actual_exit_code" -eq "$expected_exit_code" ]; then
        echo -e "${GREEN}PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        echo "  Expected exit code: $expected_exit_code"
        echo "  Actual exit code: $actual_exit_code"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Function to cleanup test repositories
cleanup_test_repos() {
    echo "Cleaning up test repositories..."
    rm -rf "$PROJECT_DIR/.cache" 2>/dev/null || true
    rm -rf "$PROJECT_DIR/github" 2>/dev/null || true
}

# Check if binary exists
if [ ! -f "$BINARY" ]; then
    echo -e "${RED}Error: Binary not found at $BINARY${NC}"
    echo "Please run 'make cache' first to build the git-cache program."
    exit 1
fi

echo -e "${BLUE}Starting git-cache integration tests...${NC}"
echo

# Clean up any existing test repositories
cleanup_test_repos

# Test 1: Help and version
run_test "Help option" 0 "$BINARY --help"
run_test "Version option" 0 "$BINARY --version"

# Test 2: Status without cache
run_test "Status command" 0 "$BINARY status"

# Test 3: Basic clone operation
echo -e "${YELLOW}Testing basic clone operation...${NC}"
run_test "Basic clone" 0 "$BINARY clone https://github.com/octocat/Hello-World.git"

# Verify cache structure was created
if [ -d "$PROJECT_DIR/.cache/git/github.com/octocat/Hello-World" ]; then
    echo -e "${GREEN}✓ Cache directory created${NC}"
else
    echo -e "${RED}✗ Cache directory not created${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

if [ -d "$PROJECT_DIR/github/octocat/Hello-World" ]; then
    echo -e "${GREEN}✓ Checkout directory created${NC}"
else
    echo -e "${RED}✗ Checkout directory not created${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

if [ -d "$PROJECT_DIR/github/mithro/octocat-Hello-World" ]; then
    echo -e "${GREEN}✓ Modifiable directory created${NC}"
else
    echo -e "${RED}✗ Modifiable directory not created${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 4: Cache hit detection
echo -e "${YELLOW}Testing cache hit detection...${NC}"
run_test "Cache hit detection" 0 "$BINARY clone https://github.com/octocat/Hello-World.git"

# Test 5: Different strategies
echo -e "${YELLOW}Testing clone strategies...${NC}"
cleanup_test_repos
run_test "Shallow clone" 0 "$BINARY clone --strategy shallow --depth 1 https://github.com/octocat/Hello-World.git"

cleanup_test_repos  
run_test "Treeless clone" 0 "$BINARY clone --strategy treeless https://github.com/octocat/Hello-World.git"

cleanup_test_repos
run_test "Blobless clone" 0 "$BINARY clone --strategy blobless https://github.com/octocat/Hello-World.git"

# Test 6: Git alternates verification
echo -e "${YELLOW}Testing git alternates configuration...${NC}"
cleanup_test_repos
$BINARY clone https://github.com/octocat/Hello-World.git >/dev/null 2>&1

if [ -f "$PROJECT_DIR/github/octocat/Hello-World/.git/objects/info/alternates" ]; then
    ALTERNATES_PATH=$(cat "$PROJECT_DIR/github/octocat/Hello-World/.git/objects/info/alternates")
    EXPECTED_PATH="$PROJECT_DIR/.cache/git/github.com/octocat/Hello-World/objects"
    if [ "$ALTERNATES_PATH" = "$EXPECTED_PATH" ]; then
        echo -e "${GREEN}✓ Git alternates correctly configured${NC}"
    else
        echo -e "${RED}✗ Git alternates path incorrect${NC}"
        echo "  Expected: $EXPECTED_PATH"
        echo "  Actual: $ALTERNATES_PATH"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
else
    echo -e "${RED}✗ Git alternates file not found${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 7: GitHub integration (if token available)
if [ -n "$GITHUB_TOKEN" ]; then
    echo -e "${YELLOW}Testing GitHub integration...${NC}"
    cleanup_test_repos
    run_test "GitHub clone with token" 0 "$BINARY clone --verbose https://github.com/octocat/Hello-World.git"
    
    echo -e "${YELLOW}Note: Fork creation testing requires specific repository permissions${NC}"
else
    echo -e "${YELLOW}Skipping GitHub integration tests (no GITHUB_TOKEN)${NC}"
fi

# Test 8: Error handling
echo -e "${YELLOW}Testing error handling...${NC}"
run_test "Invalid URL" 1 "$BINARY clone invalid-url"
run_test "Non-existent repository" 1 "$BINARY clone https://github.com/nonexistent/repo.git"

# Clean up test repositories
cleanup_test_repos

echo
echo "Git Cache Test Summary:"
echo -e "  Total tests: $TESTS_RUN"
echo -e "  ${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "  ${RED}Failed: $TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All git-cache tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some git-cache tests failed.${NC}"
    exit 1
fi