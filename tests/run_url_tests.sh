#!/bin/bash

# URL parsing test runner

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
URL_TEST_BINARY="$PROJECT_DIR/test_url_parsing"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if binary exists
if [ ! -f "$URL_TEST_BINARY" ]; then
	echo -e "${RED}Error: URL test binary not found at $URL_TEST_BINARY${NC}"
	echo "Please run 'make url-test' first to build the test_url_parsing program."
	exit 1
fi

echo -e "${BLUE}Starting URL parsing tests...${NC}"
echo

# Run the URL parsing test suite
if "$URL_TEST_BINARY"; then
	echo -e "${GREEN}URL parsing tests passed!${NC}"
	exit 0
else
	echo -e "${RED}URL parsing tests failed.${NC}"
	exit 1
fi