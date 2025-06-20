# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Commands

### Building
```bash
make cache          # Build main git-cache binary
make github         # Build GitHub API test binary
make url-test       # Build URL parsing test binary
make clean          # Remove all compiled objects and binaries
```

### Testing
```bash
make test-all           # Run complete test suite (all 4 test categories)
make cache-test         # Run git-cache integration tests
make github-test        # Run GitHub API tests  
make url-test-run       # Run URL parsing tests
make robustness-test    # Run robustness and failure recovery tests
make concurrent-test    # Run concurrent execution tests
./test_concurrent.sh    # Direct concurrent execution test
```

### Cache Management During Development
```bash
make clean-cache        # Remove .cache and github directories
make clean-all          # Full cleanup (clean + clean-cache)
```

## Architecture Overview

git-cache implements a sophisticated three-tier Git repository caching system with robust concurrent execution handling.

### Core Components

**Main Application (`git-cache.c`)**
- Repository cache management with atomic operations
- File-based locking system for concurrent execution safety
- Backup/restore mechanisms for corruption recovery
- Reference-based checkout creation using Git alternates
- Environment variable configuration system

**GitHub Integration (`github_api.c` + `github_api.h`)**
- Comprehensive URL parsing for all Git URL formats
- GitHub API client with authentication and error handling
- Automatic fork creation and repository management
- Repository metadata extraction and validation

### Repository Structure

git-cache creates a three-tier hierarchy:

1. **Cache Tier** (`~/.cache/git/github.com/owner/repo`): Bare repositories for storage efficiency
2. **Read-only Tier** (`./github/owner/repo`): Reference clones for browsing  
3. **Modifiable Tier** (`./github/mithro/owner-repo`): Development copies with space-saving strategies

### Concurrent Execution System

**File-Based Locking**
- Uses `.lock` files with PID tracking for ownership verification
- 5-minute timeout for stale lock detection based on file modification time
- 60-second maximum wait with 100ms retry intervals
- Automatic cleanup of orphaned locks from terminated processes

**Atomic Operations**
- Temporary files with timestamp suffixes for atomic moves
- Backup/restore mechanisms for recovery from failures
- Lock cleanup macros ensure no leaked locks on function exit

### Clone Strategies

- **Full**: Complete repository clone
- **Shallow**: Limited history depth (configurable)
- **Treeless**: Objects without trees for faster clones
- **Blobless**: Objects without blobs for minimal storage

### Error Handling System

Comprehensive error codes and graceful degradation:
- Network failures preserve existing caches
- Repository corruption triggers automatic backup/restore
- Invalid configurations provide helpful error messages
- Lock contention handled with exponential backoff

### Test Architecture

**Test Categories**
- URL parsing: 33 tests covering all Git URL formats
- GitHub API: Authentication and repository operations
- Integration: End-to-end clone operations and cache behavior
- Robustness: 7 comprehensive failure recovery scenarios
- Concurrent: File locking and race condition prevention

**Test Infrastructure**
- Isolated test environments with automatic cleanup
- Parallel test execution capabilities
- CI integration with GitHub Actions
- Comprehensive coverage of edge cases and error conditions

### Configuration System

Environment variables override defaults:
- `GIT_CACHE`: Cache directory location
- `GIT_CHECKOUT_ROOT`: Checkout base directory  
- `GITHUB_TOKEN`: API authentication for private repos
- Project-local configuration via `GIT_CACHE` during testing

### Code Style Requirements

- Git project coding style (tabs for indentation, no spaces)
- C99 standard compliance with `-Wall -Wextra -pedantic`
- Function naming: lowercase with underscores
- Static functions for internal use, minimal public interface
- Comprehensive error checking with meaningful messages

## Important Implementation Details

### Memory Management
All dynamic allocations have corresponding cleanup paths. Use the established patterns for string allocation and freeing.

### Lock Management
When modifying cache or checkout operations, always use the `RETURN_WITH_LOCK_CLEANUP` macro to ensure locks are released on all exit paths.

### Repository Validation
The `validate_git_repository()` function checks both bare and working tree repositories. Use this for corruption detection.

### Atomic File Operations
Follow the pattern of temporary files with timestamps, validation, then atomic move to final location.

### URL Parsing
The GitHub URL parser handles 20+ URL formats. Extend the test suite when adding new format support.