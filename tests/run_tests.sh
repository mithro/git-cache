#!/bin/bash

# Basic test runner for git-mycommand

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_DIR/git-mycommand"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
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
    echo "Please run 'make' first to build the project."
    exit 1
fi

echo "Starting tests for git-mycommand..."
echo

# Test 1: Help option
run_test "Help option (-h)" 1 "$BINARY -h"

# Test 2: Help option (--help)
run_test "Help option (--help)" 1 "$BINARY --help"

# Test 3: Unknown option
run_test "Unknown option" 1 "$BINARY --unknown-option"

# Test 4: Basic execution (will fail if not in git repo)
run_test "Basic execution outside git repo" 1 "$BINARY"

# Test 5: Verbose option outside git repo
run_test "Verbose option outside git repo" 1 "$BINARY -v"

# Create a temporary git repository for testing
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"
git init >/dev/null 2>&1
git config user.name "Test User"
git config user.email "test@example.com"

# Test 6: Basic execution in git repo
run_test "Basic execution in git repo" 0 "$BINARY"

# Test 7: Verbose option in git repo
run_test "Verbose option in git repo" 0 "$BINARY -v"

# Test 8: With arguments
run_test "With arguments" 0 "$BINARY arg1 arg2"

# Clean up
cd "$PROJECT_DIR"
rm -rf "$TEMP_DIR"

echo
echo "Test Summary:"
echo -e "  Total tests: $TESTS_RUN"
echo -e "  ${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "  ${RED}Failed: $TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi