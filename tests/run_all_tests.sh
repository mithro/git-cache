#!/bin/bash

# Comprehensive test runner for all git-cache components

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test suite counters
SUITES_RUN=0
SUITES_PASSED=0
SUITES_FAILED=0

# Function to run a test suite
run_test_suite() {
	local suite_name="$1"
	local script_path="$2"
	
	echo -e "${BLUE}Running $suite_name...${NC}"
	echo "======================================"
	
	SUITES_RUN=$((SUITES_RUN + 1))
	
	if [ ! -f "$script_path" ]; then
		echo -e "${RED}Error: Test script not found: $script_path${NC}"
		SUITES_FAILED=$((SUITES_FAILED + 1))
		return 1
	fi
	
	if bash "$script_path"; then
		echo -e "${GREEN}✓ $suite_name passed${NC}"
		SUITES_PASSED=$((SUITES_PASSED + 1))
		echo
		return 0
	else
		echo -e "${RED}✗ $suite_name failed${NC}"
		SUITES_FAILED=$((SUITES_FAILED + 1))
		echo
		return 1
	fi
}

echo -e "${BLUE}Git Cache - Comprehensive Test Suite${NC}"
echo "====================================="
echo

# Build all necessary binaries first
echo -e "${YELLOW}Building test binaries...${NC}"
cd "$PROJECT_DIR"
if ! make url-test >/dev/null 2>&1; then
	echo -e "${RED}Failed to build URL test binary${NC}"
	exit 1
fi

if ! make cache >/dev/null 2>&1; then
	echo -e "${RED}Failed to build git-cache binary${NC}"
	exit 1
fi

if ! make github >/dev/null 2>&1; then
	echo -e "${RED}Failed to build github test binary${NC}"
	exit 1
fi

echo -e "${GREEN}All binaries built successfully${NC}"
echo

# Run URL parsing tests first (foundational)
run_test_suite "URL Parsing Tests" "$SCRIPT_DIR/run_url_tests.sh"

# Run GitHub API tests (if github binary exists)
if [ -f "$PROJECT_DIR/github_test" ]; then
	run_test_suite "GitHub API Tests" "$SCRIPT_DIR/run_github_tests.sh"
fi

# Run git-cache integration tests
run_test_suite "Git Cache Integration Tests" "$SCRIPT_DIR/run_cache_tests.sh"

echo -e "${BLUE}Overall Test Suite Summary${NC}"
echo "=========================="
echo -e "  Total test suites: $SUITES_RUN"
echo -e "  ${GREEN}Passed: $SUITES_PASSED${NC}"
echo -e "  ${RED}Failed: $SUITES_FAILED${NC}"

if [ $SUITES_FAILED -eq 0 ]; then
	echo -e "${GREEN}🎉 All test suites passed!${NC}"
	exit 0
else
	echo -e "${RED}❌ Some test suites failed.${NC}"
	exit 1
fi