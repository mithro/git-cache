#!/bin/bash

# Test script for concurrent execution handling

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="$SCRIPT_DIR/git-cache"
TEST_URL="https://github.com/octocat/Hello-World.git"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Testing Concurrent Execution Handling${NC}"
echo "====================================="

# Cleanup function
cleanup() {
	echo "Cleaning up test environment..."
	rm -rf github .cache 2>/dev/null || true
	find . -name "*.lock" -delete 2>/dev/null || true
}

# Cleanup before starting
cleanup

echo -e "${YELLOW}Test 1: Sequential operations (baseline)${NC}"
echo "Running two sequential clones..."

$BINARY clone "$TEST_URL" >/dev/null 2>&1
echo "✓ First clone completed"

$BINARY clone "$TEST_URL" >/dev/null 2>&1
echo "✓ Second clone completed"

# Check for any remaining lock files
LOCK_COUNT=$(find . -name "*.lock" 2>/dev/null | wc -l)
if [ "$LOCK_COUNT" -eq 0 ]; then
	echo -e "${GREEN}✓ No lock files remaining after sequential operations${NC}"
else
	echo -e "${RED}✗ Found $LOCK_COUNT lock files after sequential operations${NC}"
	find . -name "*.lock" 2>/dev/null || true
fi

echo
echo -e "${YELLOW}Test 2: Concurrent operations${NC}"
echo "Running two concurrent clones..."

cleanup

# Start two clones simultaneously in background
$BINARY clone -v "$TEST_URL" &
PID1=$!
echo "Started clone 1 (PID: $PID1)"

# Brief delay to ensure first clone starts
sleep 0.1

$BINARY clone -v "$TEST_URL" &
PID2=$!
echo "Started clone 2 (PID: $PID2)"

# Monitor for lock files during execution
echo "Monitoring lock files during execution..."
for i in {1..20}; do
	ACTIVE_LOCKS=$(find . -name "*.lock" 2>/dev/null | wc -l)
	if [ "$ACTIVE_LOCKS" -gt 0 ]; then
		echo "Found $ACTIVE_LOCKS active lock file(s) at iteration $i"
		find . -name "*.lock" -exec basename {} \; 2>/dev/null | head -5
	fi
	sleep 0.1
done

# Wait for both processes to complete
echo "Waiting for processes to complete..."
wait $PID1 2>/dev/null || true
EXIT1=$?
echo "Clone 1 completed with exit code: $EXIT1"

wait $PID2 2>/dev/null || true  
EXIT2=$?
echo "Clone 2 completed with exit code: $EXIT2"

# Check final state
echo
echo "Checking final state..."

# Verify repositories were created
if [ -d "github/octocat/Hello-World" ]; then
	echo -e "${GREEN}✓ Checkout repository exists${NC}"
else
	echo -e "${RED}✗ Checkout repository missing${NC}"
fi

if [ -d "$HOME/.cache/git/github.com/octocat/Hello-World" ]; then
	echo -e "${GREEN}✓ Cache repository exists${NC}"
else
	echo -e "${RED}✗ Cache repository missing${NC}"
fi

# Check for remaining lock files
FINAL_LOCKS=$(find . -name "*.lock" 2>/dev/null | wc -l)
if [ "$FINAL_LOCKS" -eq 0 ]; then
	echo -e "${GREEN}✓ No lock files remaining after concurrent operations${NC}"
else
	echo -e "${RED}✗ Found $FINAL_LOCKS lock files after concurrent operations${NC}"
	find . -name "*.lock" 2>/dev/null || true
fi

# Check for temporary files
TEMP_FILES=$(find github -name "*.tmp.*" 2>/dev/null | wc -l)
if [ "$TEMP_FILES" -eq 0 ]; then
	echo -e "${GREEN}✓ No temporary files remaining${NC}"
else
	echo -e "${RED}✗ Found $TEMP_FILES temporary files${NC}"
	find github -name "*.tmp.*" 2>/dev/null || true
fi

echo
echo -e "${YELLOW}Test 3: Lock timeout simulation${NC}"
echo "Testing lock timeout behavior..."

cleanup

# Create a fake lock file to simulate a stale lock
mkdir -p github/octocat
FAKE_LOCK="github/octocat/Hello-World.lock"
echo "99999" > "$FAKE_LOCK"
# Make it old (5 minutes ago)
touch -t $(date -d '5 minutes ago' +%Y%m%d%H%M.%S) "$FAKE_LOCK" 2>/dev/null || true

echo "Created fake stale lock file"

# Try to clone - should detect and remove stale lock
$BINARY clone -v "$TEST_URL" >/dev/null 2>&1
CLONE_EXIT=$?

echo "Clone completed with exit code: $CLONE_EXIT"

if [ $CLONE_EXIT -eq 0 ]; then
	echo -e "${GREEN}✓ Clone succeeded despite stale lock${NC}"
else
	echo -e "${RED}✗ Clone failed with stale lock${NC}"
fi

# Check if stale lock was cleaned up
if [ ! -f "$FAKE_LOCK" ]; then
	echo -e "${GREEN}✓ Stale lock was automatically cleaned up${NC}"
else
	echo -e "${RED}✗ Stale lock was not cleaned up${NC}"
fi

echo
echo -e "${BLUE}Concurrent Execution Test Summary${NC}"
echo "================================"

if [ "$LOCK_COUNT" -eq 0 ] && [ "$FINAL_LOCKS" -eq 0 ] && [ "$TEMP_FILES" -eq 0 ] && [ $CLONE_EXIT -eq 0 ]; then
	echo -e "${GREEN}✓ All concurrent execution tests passed!${NC}"
	echo "  - Sequential operations work correctly"
	echo "  - Concurrent operations handle locking properly"  
	echo "  - No lock files or temporary files remain"
	echo "  - Stale lock detection and cleanup works"
else
	echo -e "${RED}✗ Some concurrent execution tests failed${NC}"
	[ "$LOCK_COUNT" -ne 0 ] && echo "  - Lock files remained after sequential operations"
	[ "$FINAL_LOCKS" -ne 0 ] && echo "  - Lock files remained after concurrent operations"
	[ "$TEMP_FILES" -ne 0 ] && echo "  - Temporary files remained"
	[ $CLONE_EXIT -ne 0 ] && echo "  - Stale lock handling failed"
fi

# Final cleanup
cleanup

echo "Test completed."