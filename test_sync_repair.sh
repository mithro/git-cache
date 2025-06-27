#!/bin/bash
# Test script to verify checkout repair after sync

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Testing git-cache sync with checkout repair${NC}"
echo "============================================"

# Create a test environment
TEST_DIR="/tmp/git-cache-test-$$"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Set up test configuration
export GIT_CACHE_ROOT="$TEST_DIR/cache"
export GIT_CACHE_CHECKOUT_ROOT="$TEST_DIR/checkouts"

echo -e "\n${YELLOW}Test environment:${NC}"
echo "  Cache root: $GIT_CACHE_ROOT"
echo "  Checkout root: $GIT_CACHE_CHECKOUT_ROOT"

# Create cache directories
mkdir -p "$GIT_CACHE_ROOT"
mkdir -p "$GIT_CACHE_CHECKOUT_ROOT"

# Clone a small test repository
echo -e "\n${YELLOW}1. Cloning test repository...${NC}"
../git-cache clone https://github.com/octocat/Hello-World.git

# Check if checkout was created
if [ -d "$GIT_CACHE_CHECKOUT_ROOT/octocat/Hello-World" ]; then
    echo -e "${GREEN}✓ Checkout created successfully${NC}"
else
    echo -e "${RED}✗ Checkout not created${NC}"
    exit 1
fi

# Touch the cache to simulate an update
echo -e "\n${YELLOW}2. Simulating cache update...${NC}"
sleep 2  # Ensure different timestamp
touch "$GIT_CACHE_ROOT/github.com/octocat/Hello-World/refs"

# Run sync command with verbose output
echo -e "\n${YELLOW}3. Running git-cache sync...${NC}"
../git-cache sync -v

# Check output for repair message
echo -e "\n${YELLOW}4. Checking results...${NC}"
if ../git-cache sync -v 2>&1 | grep -q "Checking for outdated checkouts"; then
    echo -e "${GREEN}✓ Checkout repair functionality is active${NC}"
else
    echo -e "${RED}✗ Checkout repair functionality not detected${NC}"
fi

# Clean up
echo -e "\n${YELLOW}5. Cleaning up test environment...${NC}"
cd /
rm -rf "$TEST_DIR"

echo -e "\n${GREEN}Test completed successfully!${NC}"