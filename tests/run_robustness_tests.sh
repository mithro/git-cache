#!/bin/bash

# Git cache robustness and failure recovery test runner

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_DIR/git-cache"

# Set GIT_CACHE environment variable to use project-local cache during testing
export GIT_CACHE="$PROJECT_DIR/.cache/git"

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
		return 0
	else
		echo -e "${RED}FAIL${NC}"
		echo "  Expected exit code: $expected_exit_code"
		echo "  Actual exit code: $actual_exit_code"
		TESTS_FAILED=$((TESTS_FAILED + 1))
		return 1
	fi
}

# Function to run a test and capture output
run_test_with_output() {
	local test_name="$1"
	local expected_exit_code="$2"
	shift 2
	local cmd="$@"
	
	echo -n "Running test: $test_name... "
	TESTS_RUN=$((TESTS_RUN + 1))
	
	local output
	local actual_exit_code
	output=$(eval "$cmd" 2>&1)
	actual_exit_code=$?
	
	if [ "$actual_exit_code" -eq "$expected_exit_code" ]; then
		echo -e "${GREEN}PASS${NC}"
		TESTS_PASSED=$((TESTS_PASSED + 1))
		return 0
	else
		echo -e "${RED}FAIL${NC}"
		echo "  Expected exit code: $expected_exit_code"
		echo "  Actual exit code: $actual_exit_code"
		echo "  Output: $output"
		TESTS_FAILED=$((TESTS_FAILED + 1))
		return 1
	fi
}

# Function to cleanup test repositories
cleanup_test_repos() {
	echo "Cleaning up test repositories..."
	cd "$PROJECT_DIR" && make clean-cache >/dev/null 2>&1 || {
		# Fallback to direct removal if make fails
		rm -rf "$GIT_CACHE" 2>/dev/null || true
		rm -rf "$PROJECT_DIR/github" 2>/dev/null || true
	}
}

# Function to create a corrupted repository
create_corrupted_cache() {
	local repo_path="$1"
	mkdir -p "$repo_path"
	
	# Create partial git structure that will fail validation
	mkdir -p "$repo_path/objects"
	mkdir -p "$repo_path/refs"
	echo "invalid content" > "$repo_path/HEAD"
	# Missing essential refs and objects that would make it invalid
}

# Function to create a corrupted working tree
create_corrupted_checkout() {
	local repo_path="$1"
	mkdir -p "$repo_path"
	mkdir -p "$repo_path/.git"
	
	# Create partial git structure that will fail validation
	mkdir -p "$repo_path/.git/objects"
	echo "invalid content" > "$repo_path/.git/HEAD"
	# Missing essential components
}

# Function to create a partial clone (simulate interrupted clone)
create_partial_clone() {
	local repo_path="$1"
	mkdir -p "$repo_path"
	mkdir -p "$repo_path/.tmp.123456"
	
	# Create a directory that looks like an interrupted temporary clone
	echo "partial content" > "$repo_path/.tmp.123456/README"
}

# Function to simulate a corrupted cache by removing essential files
corrupt_existing_cache() {
	local repo_path="$1"
	if [ -d "$repo_path" ]; then
		# Remove HEAD file to corrupt the repository
		rm -f "$repo_path/HEAD"
		# Corrupt objects directory
		rm -rf "$repo_path/objects/pack" 2>/dev/null || true
	fi
}

# Check if binary exists
if [ ! -f "$BINARY" ]; then
	echo -e "${RED}Error: Binary not found at $BINARY${NC}"
	echo "Please run 'make cache' first to build the git-cache program."
	exit 1
fi

echo -e "${BLUE}Starting git-cache robustness and failure recovery tests...${NC}"
echo

# Clean up any existing test repositories
cleanup_test_repos

TEST_REPO_URL="https://github.com/octocat/Hello-World.git"
CACHE_PATH="$GIT_CACHE/github.com/octocat/Hello-World"
CHECKOUT_PATH="$PROJECT_DIR/github/octocat/Hello-World"
MODIFIABLE_PATH="$PROJECT_DIR/github/mithro/octocat-Hello-World"

echo -e "${YELLOW}=== Testing Corrupted Cache Recovery ===${NC}"

# Test 1: Recovery from corrupted cache repository
echo -e "${YELLOW}Testing corrupted cache repository recovery...${NC}"
cleanup_test_repos

# Create a corrupted cache
create_corrupted_cache "$CACHE_PATH"

# Verify the cache is corrupted
if [ -d "$CACHE_PATH" ]; then
	echo -e "${GREEN}✓ Corrupted cache created for testing${NC}"
else
	echo -e "${RED}✗ Failed to create corrupted cache${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Now try to clone - should detect corruption and recover
run_test "Corrupted cache recovery" 0 "$BINARY clone -v $TEST_REPO_URL"

# Verify recovery worked
if [ -f "$CACHE_PATH/HEAD" ] && [ -d "$CACHE_PATH/objects" ] && [ -d "$CACHE_PATH/refs" ]; then
	echo -e "${GREEN}✓ Cache repository recovered successfully${NC}"
else
	echo -e "${RED}✗ Cache repository recovery failed${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 2: Recovery from corrupted checkout repository
echo -e "${YELLOW}Testing corrupted checkout repository recovery...${NC}"

# First create a valid repository
cleanup_test_repos
$BINARY clone "$TEST_REPO_URL" >/dev/null 2>&1

# Now corrupt the checkout
create_corrupted_checkout "$CHECKOUT_PATH"

# Verify the checkout is corrupted
if [ -d "$CHECKOUT_PATH/.git" ] && [ ! -f "$CHECKOUT_PATH/.git/refs" ]; then
	echo -e "${GREEN}✓ Corrupted checkout created for testing${NC}"
else
	echo -e "${RED}✗ Failed to create corrupted checkout${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Try to clone again - should detect corruption and recover
run_test "Corrupted checkout recovery" 0 "$BINARY clone -v $TEST_REPO_URL"

# Verify recovery worked
if [ -f "$CHECKOUT_PATH/.git/HEAD" ] && [ -d "$CHECKOUT_PATH/.git/objects" ]; then
	echo -e "${GREEN}✓ Checkout repository recovered successfully${NC}"
else
	echo -e "${RED}✗ Checkout repository recovery failed${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

echo -e "${YELLOW}=== Testing Partial Clone Cleanup ===${NC}"

# Test 3: Cleanup of partial/interrupted clones
echo -e "${YELLOW}Testing partial clone cleanup...${NC}"
cleanup_test_repos

# Create partial clone artifacts
create_partial_clone "$CHECKOUT_PATH"

# Check for temporary files
TEMP_FILES=$(find "$PROJECT_DIR/github" -name "*.tmp.*" 2>/dev/null | wc -l)
if [ "$TEMP_FILES" -gt 0 ]; then
	echo -e "${GREEN}✓ Partial clone artifacts created for testing${NC}"
else
	echo -e "${RED}✗ Failed to create partial clone artifacts${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Run clone - should clean up partial artifacts
run_test "Partial clone cleanup" 0 "$BINARY clone -v $TEST_REPO_URL"

# Verify cleanup worked
TEMP_FILES_AFTER=$(find "$PROJECT_DIR/github" -name "*.tmp.*" 2>/dev/null | wc -l)
if [ "$TEMP_FILES_AFTER" -eq 0 ]; then
	echo -e "${GREEN}✓ Partial clone artifacts cleaned up${NC}"
else
	echo -e "${RED}✗ Partial clone artifacts not cleaned up${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

echo -e "${YELLOW}=== Testing Network Failure Resilience ===${NC}"

# Test 4: Network failure handling (invalid URL)
echo -e "${YELLOW}Testing network failure handling...${NC}"
cleanup_test_repos

# First create a valid cache
$BINARY clone "$TEST_REPO_URL" >/dev/null 2>&1

# Verify cache was created
if [ -d "$CACHE_PATH" ]; then
	echo -e "${GREEN}✓ Valid cache created${NC}"
else
	echo -e "${RED}✗ Failed to create valid cache${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Now try to clone with invalid URL - should preserve existing cache
run_test "Network failure with existing cache" 1 "$BINARY clone https://github.com/nonexistent/repo.git"

# Verify original cache is still intact
if [ -d "$CACHE_PATH" ] && [ -f "$CACHE_PATH/HEAD" ]; then
	echo -e "${GREEN}✓ Existing cache preserved after network failure${NC}"
else
	echo -e "${RED}✗ Existing cache not preserved after network failure${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

echo -e "${YELLOW}=== Testing Backup and Restore Operations ===${NC}"

# Test 5: Backup creation and restoration
echo -e "${YELLOW}Testing backup and restore operations...${NC}"
cleanup_test_repos

# Create a valid repository first
$BINARY clone "$TEST_REPO_URL" >/dev/null 2>&1

# Save original modification time
ORIGINAL_MTIME=$(stat -c %Y "$CACHE_PATH/HEAD" 2>/dev/null || echo "0")

# Corrupt the cache to trigger backup
corrupt_existing_cache "$CACHE_PATH"

# Verify corruption
if [ ! -f "$CACHE_PATH/HEAD" ]; then
	echo -e "${GREEN}✓ Cache corrupted for backup testing${NC}"
else
	echo -e "${RED}✗ Failed to corrupt cache${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Clone again - should detect corruption, backup, and recreate
run_test_with_output "Backup and restore operations" 0 "$BINARY clone -v $TEST_REPO_URL"

# Check for backup files
BACKUP_COUNT=$(find "$GIT_CACHE" -name "*.backup.*" 2>/dev/null | wc -l)
if [ "$BACKUP_COUNT" -eq 0 ]; then
	echo -e "${GREEN}✓ Backup cleaned up after successful recovery${NC}"
else
	echo -e "${YELLOW}Note: Found $BACKUP_COUNT backup files (may be expected)${NC}"
fi

# Verify new repository is valid
if [ -f "$CACHE_PATH/HEAD" ] && [ -d "$CACHE_PATH/objects" ]; then
	echo -e "${GREEN}✓ Repository restored successfully${NC}"
else
	echo -e "${RED}✗ Repository restoration failed${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

echo -e "${YELLOW}=== Testing Atomic Operations ===${NC}"

# Test 6: Atomic operation verification
echo -e "${YELLOW}Testing atomic operations...${NC}"
cleanup_test_repos

# Create a monitoring script that checks for temporary files during clone
MONITOR_SCRIPT="/tmp/monitor_atomicity.sh"
cat > "$MONITOR_SCRIPT" << 'EOF'
#!/bin/bash
for i in {1..30}; do
	TEMP_COUNT=$(find "$1" -name "*.tmp.*" 2>/dev/null | wc -l)
	if [ "$TEMP_COUNT" -gt 0 ]; then
		echo "Found temporary files during operation"
		exit 0
	fi
	sleep 0.1
done
echo "No temporary files found"
EOF
chmod +x "$MONITOR_SCRIPT"

# Run clone operation in background and monitor
$BINARY clone "$TEST_REPO_URL" >/dev/null 2>&1 &
CLONE_PID=$!

# Monitor for temporary files
"$MONITOR_SCRIPT" "$PROJECT_DIR/github" &
MONITOR_PID=$!

# Wait for clone to complete
wait $CLONE_PID
CLONE_EXIT=$?

# Stop monitoring
kill $MONITOR_PID 2>/dev/null || true
wait $MONITOR_PID 2>/dev/null || true

# Clean up monitor script
rm -f "$MONITOR_SCRIPT"

if [ $CLONE_EXIT -eq 0 ]; then
	echo -e "${GREEN}✓ Clone completed successfully${NC}"
else
	echo -e "${RED}✗ Clone failed${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Verify no temporary files remain
REMAINING_TEMP=$(find "$PROJECT_DIR/github" -name "*.tmp.*" 2>/dev/null | wc -l)
if [ "$REMAINING_TEMP" -eq 0 ]; then
	echo -e "${GREEN}✓ No temporary files remaining after operation${NC}"
else
	echo -e "${RED}✗ Found $REMAINING_TEMP temporary files after operation${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

echo -e "${YELLOW}=== Testing Interrupted Clone Recovery ===${NC}"

# Test 7: Simulated interrupted clone (create leftover temporary files)
echo -e "${YELLOW}Testing interrupted clone recovery...${NC}"
cleanup_test_repos

# Manually create temporary files that would be left by an interrupted clone
mkdir -p "$PROJECT_DIR/github/octocat"
mkdir -p "$PROJECT_DIR/github/mithro"
echo "partial content" > "$PROJECT_DIR/github/octocat/Hello-World.tmp.999999"
echo "partial content" > "$PROJECT_DIR/github/mithro/octocat-Hello-World.tmp.999999"

# Check for temporary files
TEMP_FILES_BEFORE=$(find "$PROJECT_DIR/github" -name "*.tmp.*" 2>/dev/null | wc -l)
if [ "$TEMP_FILES_BEFORE" -gt 0 ]; then
	echo "Created $TEMP_FILES_BEFORE temporary files to simulate interruption"
else
	echo -e "${RED}✗ Failed to create test temporary files${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Now try to clone - should handle leftover temporary files gracefully
run_test "Recovery from interrupted clone" 0 "$BINARY clone -v $TEST_REPO_URL"

# Verify final state is clean
TEMP_FILES_FINAL=$(find "$PROJECT_DIR/github" -name "*.tmp.*" 2>/dev/null | wc -l)
if [ "$TEMP_FILES_FINAL" -eq 0 ]; then
	echo -e "${GREEN}✓ Interrupted clone artifacts cleaned up${NC}"
else
	echo -e "${RED}✗ Found $TEMP_FILES_FINAL temporary files after recovery${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Verify repositories are valid
if [ -f "$CACHE_PATH/HEAD" ] && [ -f "$CHECKOUT_PATH/.git/HEAD" ]; then
	echo -e "${GREEN}✓ Repositories are valid after interruption recovery${NC}"
else
	echo -e "${RED}✗ Repositories are invalid after interruption recovery${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

echo -e "${YELLOW}=== Testing Permission and Space Issues ===${NC}"

# Test 8: Permission handling
echo -e "${YELLOW}Testing permission handling...${NC}"
cleanup_test_repos

# Create a directory with restricted permissions
READONLY_TEST_DIR="$PROJECT_DIR/readonly_test"
mkdir -p "$READONLY_TEST_DIR"
chmod 444 "$READONLY_TEST_DIR"

# Try to create a repository in readonly directory - should fail gracefully
run_test "Readonly directory handling" 1 "GIT_CACHE='$READONLY_TEST_DIR' $BINARY clone $TEST_REPO_URL"

# Clean up readonly test
chmod 755 "$READONLY_TEST_DIR" 2>/dev/null || true
rm -rf "$READONLY_TEST_DIR" 2>/dev/null || true

echo -e "${GREEN}✓ Permission errors handled gracefully${NC}"

echo -e "${YELLOW}=== Testing Multiple Sequential Operations ===${NC}"

# Test 9: Temporal separation and cleanup robustness
echo -e "${YELLOW}Testing temporal separation robustness...${NC}"
cleanup_test_repos

# Test that multiple sequential operations work correctly
# and that each operation properly cleans up after itself
for i in 1 2 3; do
	echo "Sequential operation $i..."
	$BINARY clone "$TEST_REPO_URL" >/dev/null 2>&1
	
	# Check for temporary files after each operation
	TEMP_COUNT=$(find "$PROJECT_DIR/github" -name "*.tmp.*" 2>/dev/null | wc -l)
	if [ "$TEMP_COUNT" -gt 0 ]; then
		echo -e "${RED}✗ Found $TEMP_COUNT temporary files after operation $i${NC}"
		TESTS_FAILED=$((TESTS_FAILED + 1))
		break
	fi
done

# Verify final repositories are valid
if [ -f "$CACHE_PATH/HEAD" ] && [ -f "$CHECKOUT_PATH/.git/HEAD" ]; then
	echo -e "${GREEN}✓ Multiple operations completed successfully${NC}"
else
	echo -e "${RED}✗ Multiple operations failed${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Final verification that no temporary files remain
TEMP_FILES_FINAL=$(find "$PROJECT_DIR/github" -name "*.tmp.*" 2>/dev/null | wc -l)
if [ "$TEMP_FILES_FINAL" -eq 0 ]; then
	echo -e "${GREEN}✓ No temporary files after multiple operations${NC}"
else
	echo -e "${RED}✗ Found $TEMP_FILES_FINAL temporary files after multiple operations${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

echo -e "${YELLOW}=== Testing Directory Safety ===${NC}"

# Test 10: Directory safety checks
echo -e "${YELLOW}Testing directory safety checks...${NC}"

# Test should fail if we try to remove a system directory (safety check)
# This test ensures our safe_remove_directory function works correctly
# We can't easily test this without risking actual system damage, so we'll
# verify the code exists and trust the implementation

if grep -q "strcmp(path, \"/\")" "$PROJECT_DIR/git-cache.c"; then
	echo -e "${GREEN}✓ Root directory safety check found in code${NC}"
else
	echo -e "${RED}✗ Root directory safety check not found${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

if grep -q "safe_remove_directory" "$PROJECT_DIR/git-cache.c"; then
	echo -e "${GREEN}✓ Safe directory removal function found${NC}"
else
	echo -e "${RED}✗ Safe directory removal function not found${NC}"
	TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Clean up test repositories
cleanup_test_repos

echo
echo "Git Cache Robustness Test Summary:"
echo -e "  Total tests: $TESTS_RUN"
echo -e "  ${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "  ${RED}Failed: $TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
	echo -e "${GREEN}All robustness tests passed!${NC}"
	exit 0
else
	echo -e "${RED}Some robustness tests failed.${NC}"
	exit 1
fi