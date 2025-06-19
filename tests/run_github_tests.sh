#!/bin/bash

# GitHub API integration test runner

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_DIR/github_test"

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

# Check if binary exists
if [ ! -f "$BINARY" ]; then
    echo -e "${RED}Error: Binary not found at $BINARY${NC}"
    echo "Please run 'make github' first to build the GitHub test program."
    exit 1
fi

echo -e "${BLUE}Starting GitHub API integration tests...${NC}"
echo

# Test 1: Basic functionality (no token)
run_test "Basic functionality without token" 0 "$BINARY"

# Test 2: Help option
run_test "Help option" 0 "$BINARY --help"

# Check if GitHub token is provided via environment variable
if [ -n "$GITHUB_TOKEN" ]; then
    echo -e "${YELLOW}Found GITHUB_TOKEN environment variable, running authenticated tests...${NC}"
    
    # Test 3: Token validation
    run_test "Token validation" 0 "$BINARY $GITHUB_TOKEN"
    
    # Test 4: Repository operations with token
    run_test "Repository operations with valid token" 0 "$BINARY $GITHUB_TOKEN"
    
    # If test repository is specified, test forking
    if [ -n "$TEST_REPO_OWNER" ] && [ -n "$TEST_REPO_NAME" ]; then
        echo -e "${YELLOW}Found TEST_REPO_OWNER and TEST_REPO_NAME, running fork tests...${NC}"
        run_test "Fork operations" 0 "$BINARY $GITHUB_TOKEN $TEST_REPO_OWNER $TEST_REPO_NAME"
    else
        echo -e "${YELLOW}To test forking, set TEST_REPO_OWNER and TEST_REPO_NAME environment variables${NC}"
    fi
else
    echo -e "${YELLOW}No GITHUB_TOKEN environment variable found${NC}"
    echo -e "${YELLOW}To run authenticated tests, set GITHUB_TOKEN environment variable${NC}"
    echo -e "${YELLOW}Example: export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx${NC}"
fi

echo
echo "GitHub API Test Summary:"
echo -e "  Total tests: $TESTS_RUN"
echo -e "  ${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "  ${RED}Failed: $TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All GitHub API tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some GitHub API tests failed.${NC}"
    exit 1
fi