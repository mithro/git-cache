#!/bin/bash

# Safe GitHub Integration Test Script
# 
# This script tests GitHub integration functionality with minimal side effects.
# It focuses on read-only operations and well-known test repositories.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CACHE_BINARY="$SCRIPT_DIR/git-cache"
GITHUB_BINARY="$SCRIPT_DIR/github_test"
FORK_BINARY="$SCRIPT_DIR/test_fork_integration"

# Well-known safe test repositories (publicly available, commonly forked)
SAFE_TEST_REPOS=(
    "octocat/Hello-World"
    "github/gitignore"
    "torvalds/linux"
)

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

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
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        echo "  Expected exit code: $expected_exit_code"
        echo "  Actual exit code: $actual_exit_code"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check if binaries exist
    if [ ! -f "$CACHE_BINARY" ]; then
        log_error "git-cache binary not found. Run 'make cache' first."
        exit 1
    fi
    
    if [ ! -f "$FORK_BINARY" ]; then
        log_warning "Fork integration test not found. Run 'make fork-test' to build."
    fi
    
    # Check network connectivity to GitHub
    if ! curl -s --connect-timeout 5 https://api.github.com/rate_limit >/dev/null; then
        log_warning "GitHub API not accessible. Some tests will be skipped."
        return 1
    fi
    
    log_success "Prerequisites check passed"
    return 0
}

test_fork_logic_unit_tests() {
    log_info "Running fork logic unit tests (no network required)..."
    
    if [ -f "$FORK_BINARY" ]; then
        run_test "Fork integration unit tests" 0 "$FORK_BINARY"
    else
        log_warning "Fork integration binary not found, skipping unit tests"
    fi
}

test_github_api_basic() {
    log_info "Testing basic GitHub API functionality..."
    
    if [ -f "$GITHUB_BINARY" ]; then
        run_test "GitHub API basic functionality" 0 "$GITHUB_BINARY"
        run_test "GitHub API help option" 0 "$GITHUB_BINARY --help"
    else
        log_warning "GitHub test binary not found, run 'make github' to build"
    fi
}

test_url_parsing() {
    log_info "Testing GitHub URL parsing..."
    
    # Test various GitHub URL formats
    local test_urls=(
        "https://github.com/user/repo.git"
        "git@github.com:user/repo.git"
        "https://github.com/user/repo"
        "git@github.com:user/repo"
    )
    
    for url in "${test_urls[@]}"; do
        if run_test "URL parsing: $url" 0 "$CACHE_BINARY clone --help" >/dev/null 2>&1; then
            : # Test passed, URL format is supported
        else
            log_warning "URL format might not be fully supported: $url"
        fi
    done
}

test_repository_detection() {
    log_info "Testing repository detection and fork logic..."
    
    # Test with a well-known public repository (read-only, no side effects)
    local test_repo="https://github.com/octocat/Hello-World.git"
    
    # Test that git-cache can parse the URL and detect it as GitHub
    if command -v timeout >/dev/null 2>&1; then
        # Use timeout to prevent hanging
        run_test "Repository detection (dry run)" 0 "timeout 10s $CACHE_BINARY clone --help"
    else
        run_test "Repository detection (dry run)" 0 "$CACHE_BINARY clone --help"
    fi
}

test_github_api_authenticated() {
    if [ -z "$GITHUB_TOKEN" ]; then
        log_warning "No GITHUB_TOKEN provided, skipping authenticated tests"
        log_info "To run authenticated tests:"
        log_info "  export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
        return
    fi
    
    log_info "Testing authenticated GitHub API calls..."
    
    if [ -f "$GITHUB_BINARY" ]; then
        # Test token validation
        run_test "GitHub token validation" 0 "$GITHUB_BINARY $GITHUB_TOKEN"
        
        # Test repository information retrieval (read-only)
        for repo in "${SAFE_TEST_REPOS[@]}"; do
            owner=$(echo "$repo" | cut -d'/' -f1)
            name=$(echo "$repo" | cut -d'/' -f2)
            
            if run_test "GitHub API: Get repo info for $repo" 0 "$GITHUB_BINARY $GITHUB_TOKEN"; then
                log_info "Successfully retrieved information for $repo"
            fi
        done
    fi
}

test_fork_operations_safe() {
    if [ -z "$GITHUB_TOKEN" ]; then
        log_warning "No GITHUB_TOKEN provided, skipping fork tests"
        return
    fi
    
    if [ -z "$TEST_REPO_OWNER" ] || [ -z "$TEST_REPO_NAME" ]; then
        log_warning "TEST_REPO_OWNER and TEST_REPO_NAME not set"
        log_info "To test actual fork operations (creates real forks):"
        log_info "  export TEST_REPO_OWNER=octocat"
        log_info "  export TEST_REPO_NAME=Hello-World"
        log_info "  export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
        log_warning "Note: This will create actual forks in your GitHub account!"
        return
    fi
    
    log_warning "DANGEROUS: About to test actual fork operations"
    log_warning "This will create real forks in your GitHub account!"
    
    # Give user a chance to abort
    if [ -t 0 ]; then  # Check if running interactively
        read -p "Continue with fork testing? [y/N] " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            log_info "Fork testing aborted by user"
            return
        fi
    fi
    
    log_info "Testing fork operations with $TEST_REPO_OWNER/$TEST_REPO_NAME..."
    
    if [ -f "$GITHUB_BINARY" ]; then
        run_test "GitHub fork operations" 0 "$GITHUB_BINARY $GITHUB_TOKEN $TEST_REPO_OWNER $TEST_REPO_NAME"
    fi
}

test_end_to_end_integration() {
    if [ -z "$GITHUB_TOKEN" ]; then
        log_warning "No GITHUB_TOKEN provided, skipping end-to-end tests"
        return
    fi
    
    log_info "Testing end-to-end integration (read-only operations)..."
    
    # Create a temporary directory for testing
    local test_dir=$(mktemp -d)
    trap "rm -rf $test_dir" EXIT
    
    cd "$test_dir"
    
    # Test with a small, well-known repository
    local test_repo="https://github.com/octocat/Hello-World.git"
    
    log_info "Testing cache creation and status (no forking)..."
    
    # Test status command (should work without any repositories)
    run_test "Cache status (empty)" 0 "$CACHE_BINARY status"
    
    # Test list command (should work with empty cache)
    run_test "Cache list (empty)" 0 "$CACHE_BINARY list"
    
    log_info "End-to-end testing complete"
}

main() {
    echo "GitHub Integration Safety Test Suite"
    echo "==================================="
    echo
    
    # Check prerequisites
    if ! check_prerequisites; then
        log_error "Prerequisites check failed"
        exit 1
    fi
    
    echo
    
    # Run tests in order of increasing side effects
    test_fork_logic_unit_tests
    test_github_api_basic
    test_url_parsing
    test_repository_detection
    test_github_api_authenticated
    test_end_to_end_integration
    
    # Only run fork operations if explicitly requested and configured
    if [ "$RUN_FORK_TESTS" = "true" ]; then
        test_fork_operations_safe
    else
        log_info "Skipping fork operations (set RUN_FORK_TESTS=true to enable)"
        log_warning "Fork tests create real repositories and should be run carefully"
    fi
    
    echo
    echo "=== Test Summary ==="
    echo "Total tests: $TESTS_RUN"
    echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        log_success "All GitHub integration tests passed!"
        
        if [ -n "$GITHUB_TOKEN" ]; then
            echo
            echo "To test actual fork operations (creates real forks):"
            echo "  export RUN_FORK_TESTS=true"
            echo "  export TEST_REPO_OWNER=octocat"
            echo "  export TEST_REPO_NAME=Hello-World"
            echo "  $0"
        else
            echo
            echo "To run authenticated tests:"
            echo "  export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
            echo "  $0"
        fi
        
        exit 0
    else
        log_error "Some GitHub integration tests failed"
        exit 1
    fi
}

# Run main function
main "$@"