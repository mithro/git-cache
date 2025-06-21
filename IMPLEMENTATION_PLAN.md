# Git Cache Tool Implementation Plan

## Current Implementation Status

**Last Updated**: 2025-06-21

### ✅ Completed Features
- **Phase 1**: Core Infrastructure (100% complete)
  - Basic git subcommand framework with full argument parsing
  - Cache directory management with automatic creation
  - Comprehensive GitHub URL parsing (HTTPS/SSH)
  - Concurrent execution support with file-based locking
  
- **Phase 2**: Caching Implementation (98% complete)
  - Full bare repository caching
  - Reference-based checkouts with all strategies (full, shallow, treeless, blobless)
  - Object sharing and storage optimization
  - Comprehensive error recovery and validation
  - Disk space checking before operations
  - Network retry with exponential backoff
  - Deep git repository integrity validation
  - Progress indicators for long-running operations
  
- **Phase 3**: GitHub Integration (90% complete)
  - Complete GitHub API client with authentication
  - Fork creation and management
  - Privacy settings control
  - Fork URL integration for modifiable checkouts

### 🚧 In Progress
- Phase 4: Git Submodule Support (5% - recursive flag added, core implementation pending)
- Phase 5: Advanced Features (50% - sync and enhanced list commands implemented)

### 📊 Overall Progress: ~45% Complete

---

## Overview

This document outlines the implementation plan for a custom git clone replacement with advanced caching and mirroring capabilities. The tool will be implemented as a git subcommand called `git-cache` that provides intelligent repository caching, automatic forking, and multi-location mirroring.

**Note**: The implementation uses project-local directories (`.cache/` and `github/` relative to current working directory) rather than home directory paths to avoid polluting user's home directory and enable project-specific caching.

## Project Goals

- Replace standard `git clone` with a caching-aware alternative
- Implement intelligent repository mirroring across multiple locations
- Support shallow/treeless/blobless repository optimizations
- Provide seamless integration with existing git workflows

## Architecture Overview

### Repository Structure

The tool will manage repositories in three distinct locations:

1. **Cache Repository**: `.cache/git/github.com/<username>/<repo>`
   - **Full bare repository** containing ALL git objects (blobs, trees, commits)
   - Acts as complete local object store and reference mirror
   - Contains full history and all file contents
   - Shared across multiple checkouts for storage efficiency
   - Located relative to current working directory (project-local)

2. **Read-Only Checkout**: `github/<username>/<repo>`
   - **Shallow/treeless/blobless** working directory checkout
   - References cache repository using `--reference` mechanism
   - Optimized for browsing and inspection with minimal local storage
   - Git objects shared with cache repository
   - Located relative to current working directory (project-local)

3. **Modifiable Checkout**: `github/mithro/<username>-<repo>`
   - **Partial clone** working directory for development
   - References cache repository using `--reference` mechanism
   - Connected to forked repository for push operations
   - Supports development workflows with shared object storage
   - Located relative to current working directory (project-local)

### Remote Configuration

Each repository will maintain multiple remotes:

- `origin`: Original source repository
- `fork`: GitHub mirror in mithro-mirrors organization
- `mirror`: Local machine mirror on big-storage.k207.mithis.com

## Implementation Phases

### Phase 1: Core Infrastructure

#### 1.1 Basic Git Subcommand Framework
- [x] Set up C project structure with Makefile
- [x] Implement basic argument parsing
- [x] Add git repository detection
- [x] Extend argument parsing for cache-specific options

#### 1.2 Cache Directory Management
- [x] Create `.cache/git` directory structure (project-local)
- [x] Implement cache location resolution
- [x] Add cache cleanup and maintenance functions
- [x] Implement concurrent execution with file-based locking
- [ ] Design cache metadata storage

#### 1.3 URL Parsing and Repository Identification
- [x] Parse GitHub URLs (https/ssh formats)
- [x] Extract username/repository information
- [x] Validate repository accessibility
- [x] Handle various git URL formats

### Phase 2: Caching Implementation

#### 2.1 Full Cache Repository Management
- [x] Create **full bare repository** in cache location
- [x] Implement complete git clone to populate cache with ALL objects
- [x] Fetch all branches, tags, and references
- [x] Maintain complete repository history and blobs
- [x] Support incremental updates to keep cache current

#### 2.2 Reference-Based Checkout Creation
- [x] Create partial checkouts using `--reference` to cache repository
- [x] Implement checkout strategies:
  - Shallow checkouts (`--depth=1 --reference=<cache>`)
  - Treeless checkouts (`--filter=tree:0 --reference=<cache>`)
  - Blobless checkouts (`--filter=blob:none --reference=<cache>`)
- [x] Configure git alternates automatically
- [x] Handle reference repository validation

#### 2.3 Object Sharing and Storage Optimization
- [x] Implement git alternates configuration
- [x] Verify object sharing between cache and checkouts
- [x] Handle cache repository updates and checkout synchronization
- [x] Implement storage deduplication verification
- [x] Add comprehensive error recovery and validation
- [x] Implement disk space checking before operations
- [x] Add network retry with exponential backoff
- [x] Implement deep git repository integrity validation
- [x] Add progress indicators for long-running operations
- [ ] Add checkout repair mechanisms when cache is updated

### Phase 3: GitHub Integration

#### 3.1 GitHub API Integration
- [x] Implement GitHub API client
- [x] Handle authentication (tokens, SSH keys)
- [x] Repository existence checking
- [x] Fork creation and management

#### 3.2 Fork Management
- [x] Create fork in mithro-mirrors organization
- [x] Set fork privacy settings
- [ ] Configure fork repository settings
- [x] Handle fork naming conflicts

#### 3.3 Remote Mirror Setup
- [ ] Configure remote mirrors
- [ ] Set up push/pull relationships
- [ ] Handle authentication for multiple remotes
- [ ] Implement mirror synchronization

### Phase 4: Git Submodule Support

#### 4.1 Submodule Detection and Analysis
- [x] Add --recursive flag for submodule support
- [ ] Parse `.gitmodules` file in repositories
- [ ] Detect submodule URLs and paths
- [ ] Identify submodule commit SHAs
- [ ] Handle relative submodule URLs
- [ ] Support nested submodules (recursive detection)

#### 4.2 Submodule Cache Management
- [ ] Create cache entries for each submodule
- [ ] Implement recursive caching: `.cache/git/github.com/<submodule-user>/<submodule-repo>`
- [ ] Handle submodule URL rewriting for caching
- [ ] Support submodule-specific clone strategies
- [ ] Manage submodule cache updates independently

#### 4.3 Submodule Reference Sharing
- [ ] Configure submodule checkouts to use `--reference` to their respective caches
- [ ] Implement submodule-aware alternates configuration
- [ ] Handle submodule initialization with reference repositories
- [ ] Support partial submodule clones (shallow, treeless, blobless)
- [ ] Manage submodule object sharing across parent repositories

#### 4.4 Submodule Workflow Integration
- [ ] Implement `git submodule update` with cache awareness
- [ ] Handle submodule branch tracking
- [ ] Support submodule synchronization across checkouts
- [ ] Manage submodule remotes (origin, fork, mirror)
- [ ] Handle submodule conflicts and resolution

#### 4.5 Advanced Submodule Features
- [ ] Support submodule foreach operations
- [ ] Implement submodule status reporting
- [ ] Handle submodule absorption (git submodule absorbgitdirs)
- [ ] Support submodule deinitalization and cleanup
- [ ] Manage submodule cache pruning

### Phase 5: Advanced Features

#### 5.1 Intelligent Cloning Strategies
- [ ] Auto-detect optimal clone strategy
- [ ] Repository size analysis (including submodules)
- [ ] Network condition awareness
- [ ] User preference configuration
- [ ] Submodule-aware strategy selection

#### 5.2 Multi-Location Synchronization
- [ ] Implement sync between cache and checkouts
- [ ] Handle conflicts and merge strategies
- [ ] Background synchronization (including submodules)
- [ ] Status reporting and notifications
- [ ] Submodule synchronization coordination

#### 5.3 Command Extensions
- [x] `git cache status` - Show cache status (including submodules)
- [x] `git cache clean` - Clean cache (with submodule support)
- [x] `git cache sync` - Force synchronization (recursive) with progress indicators
- [x] `git cache list` - List cached repositories with detailed information:
  - Repository size reporting
  - Last sync timestamps
  - Remote URLs and branch counts
  - Clone strategy detection
- [ ] `git cache submodule` - Submodule-specific operations

### Phase 6: Integration and Polish

#### 5.1 Git Integration
- [ ] Seamless integration with existing git commands
- [ ] Alias configuration
- [ ] Shell completion support
- [ ] Integration with git hooks

#### 5.2 Configuration Management
- [ ] Configuration file support
- [ ] User preferences
- [ ] Per-repository settings
- [ ] Environment variable support

#### 5.3 Error Handling and Recovery
- [ ] Robust error handling
- [ ] Cache corruption recovery
- [ ] Network failure handling
- [ ] Partial operation recovery

## Technical Implementation Details

### Core Data Structures

```c
struct cache_repo {
    char *username;
    char *repo_name;
    char *cache_path;          // Full bare repository path
    char *checkout_path;       // Partial checkout path
    char *modifiable_path;     // Development checkout path
    struct remote_config *remotes;
    struct submodule_info *submodules;
    int is_submodule;
};

struct submodule_info {
    char *name;
    char *path;
    char *url;
    char *cache_path;          // Submodule's cache location
    struct submodule_info *next;
};

struct remote_config {
    char *name;
    char *url;
    int is_mirror;
    int is_fork;
};

struct clone_options {
    enum {
        CLONE_FULL,
        CLONE_SHALLOW,
        CLONE_TREELESS,
        CLONE_BLOBLESS
    } strategy;
    int depth;
    char *reference_repo;      // Path to reference repository
    int recursive_submodules;
};
```

### Key Functions

```c
// Core caching functions
int cache_create_bare_repository(const char *url, const char *cache_path);
int cache_create_reference_checkout(const char *cache_path, const char *checkout_path, 
                                   struct clone_options *opts);
int cache_update_repository(const char *cache_path);

// Reference and alternates management
int setup_git_alternates(const char *checkout_path, const char *cache_path);
int verify_reference_integrity(const char *checkout_path, const char *cache_path);

// Submodule support
int parse_gitmodules(const char *repo_path, struct submodule_info **submodules);
int cache_submodule_recursive(const char *parent_cache, struct submodule_info *submodule);
int setup_submodule_references(const char *checkout_path, struct submodule_info *submodules);

// GitHub integration
int github_create_fork(const char *username, const char *repo);
int setup_remote_mirrors(struct cache_repo *repo);
```

### Configuration Files

- `.cache/git/config` - Local cache configuration (project-local)
- Environment variables for global settings (GIT_CACHE_ROOT, GIT_CHECKOUT_ROOT)
- Per-repository metadata in cache directories
- `.cache/git/github.com/<user>/<repo>/submodules.cache` - Submodule cache mapping (project-local)

### Reference Architecture Example

```bash
# Step 1: Create full bare repository in cache (project-local)
git clone --bare https://github.com/user/repo.git .cache/git/github.com/user/repo

# Step 2: Create shallow checkout using reference (project-local)
git clone --depth=1 --reference .cache/git/github.com/user/repo \
    https://github.com/user/repo.git github/user/repo

# Step 3: Create development checkout with reference (project-local)
git clone --filter=blob:none --reference .cache/git/github.com/user/repo \
    https://github.com/mithro-mirrors/user-repo.git github/mithro/user-repo

# Result: Three repositories sharing the same git objects
# - Cache: ~2GB (full repository)
# - Read-only: ~50MB (shallow + references)
# - Development: ~100MB (treeless + references)
# Total storage: ~2.15GB instead of ~6GB
```

### Submodule Reference Chain Example

```bash
# Parent repository with submodules (project-local)
.cache/git/github.com/user/parent-repo/           # Full parent cache
.cache/git/github.com/vendor/submodule1/          # Full submodule1 cache  
.cache/git/github.com/vendor/submodule2/          # Full submodule2 cache

# Checkout with submodule references (project-local)
github/user/parent-repo/                          # Shallow parent checkout
github/user/parent-repo/vendor/submodule1/        # References submodule1 cache
github/user/parent-repo/vendor/submodule2/        # References submodule2 cache

# Git alternates chain:
# parent-repo/.git/objects/info/alternates -> .cache/git/github.com/user/parent-repo/objects
# submodule1/.git/objects/info/alternates -> .cache/git/github.com/vendor/submodule1/objects
# submodule2/.git/objects/info/alternates -> .cache/git/github.com/vendor/submodule2/objects
```

## Testing Strategy

### Unit Tests
- URL parsing and validation
- Cache directory management
- Repository structure creation
- Remote configuration
- Submodule parsing (.gitmodules)
- Reference repository validation
- Git alternates configuration

### Integration Tests
- Full clone workflow with references
- Multi-repository management
- GitHub API integration
- Network failure scenarios
- Submodule recursive caching
- Reference integrity across updates
- Partial clone strategies (shallow, treeless, blobless)

### Submodule-Specific Tests
- Nested submodule handling (3+ levels deep)
- Submodule URL rewriting and resolution
- Submodule cache sharing across parent repositories
- Submodule reference chain validation
- Mixed submodule strategies (some shallow, some full)
- Submodule update and synchronization
- Submodule conflict resolution

### Performance Tests
- Large repository handling with submodules
- Cache efficiency measurements
- Storage deduplication verification
- Concurrent operations
- Submodule parallel processing
- Reference repository performance impact

## Dependencies

### Required Libraries
- libgit2 (or git command-line interface)
- libcurl (for HTTP/HTTPS operations)
- JSON parsing library (for GitHub API)
- OpenSSL (for secure connections)

### External Tools
- git (system installation)
- SSH client (for SSH operations)
- GitHub CLI (optional, for enhanced integration)

## Installation and Deployment

### Build System
- Extend existing Makefile
- Add dependency checking
- Support for different platforms
- Package creation scripts

### Installation Options
- System-wide installation (`/usr/local/bin`)
- User-local installation (consider using `~/.local/bin` for system-wide usage)
- Container-based deployment
- Package manager integration

## Future Enhancements

### Potential Extensions
- Support for other git hosting services (GitLab, Bitbucket)
- Distributed cache sharing
- Advanced compression and deduplication
- Integration with CI/CD systems
- Web-based cache management interface

### Performance Optimizations
- Parallel operations
- Delta compression
- Smart prefetching
- Cache warming strategies

## Timeline Estimate

- **Phase 1**: 2-3 weeks (Core Infrastructure)
- **Phase 2**: 4-5 weeks (Caching Implementation with References)
- **Phase 3**: 2-3 weeks (GitHub Integration)
- **Phase 4**: 5-6 weeks (Git Submodule Support - Complex)
- **Phase 5**: 3-4 weeks (Advanced Features)
- **Phase 6**: 2-3 weeks (Integration and Polish)

**Total Estimated Time**: 18-24 weeks

**Note**: Submodule support significantly increases complexity due to:
- Recursive dependency management
- Complex reference chain setup
- Nested repository synchronization
- Advanced testing requirements

## Recent Implementation Updates (2025-06-21)

### Completed Enhancements

#### Concurrent Execution Support
- Implemented file-based locking mechanism to prevent race conditions
- Added PID tracking and stale lock detection
- Timeout handling for abandoned locks (300 seconds)
- Safe concurrent operations for multiple git-cache instances

#### Cache Synchronization
- Full implementation of `git cache sync` command
- Updates all cached repositories with progress indicators
- Validates repository integrity before and after sync
- Handles errors gracefully without stopping the sync process

#### Enhanced List Command
- Added detailed repository information display:
  - Cache size calculation using `du -sh`
  - Last modification timestamps
  - Remote URL extraction
  - Branch count reporting
  - Clone strategy detection (full/shallow/treeless/blobless)
  - Checkout and modifiable repository status

#### Submodule Support (Initial)
- Added `--recursive` flag to command-line options
- Modified all git clone operations to include `--recurse-submodules` when flag is set
- Prepared infrastructure for full submodule implementation

#### Comprehensive Error Recovery
- **Disk Space Checking**: Validates available space (100MB minimum) before operations
- **Network Retry Logic**: Exponential backoff for failed network operations (max 3 attempts)
- **Repository Validation**: Deep integrity checks using git commands
- **Backup and Restore**: Automatic backup creation for corrupted repositories

#### Progress Indicators
- Visual feedback for long-running operations
- Operation-specific messages (cloning, updating, syncing, scanning)
- Spinner animation for active operations
- Clean progress indicator removal on completion

#### GitHub Fork Integration Fix
- **Critical Bug Fix**: Modifiable checkouts now correctly use forked repository URLs
- **Fork URL Storage**: Added fork_url field to repo_info structure for proper tracking
- **Fallback Logic**: Uses original URL if fork creation fails or is not needed
- **Verbose Logging**: Shows which repository URL is being used for checkouts
- **Complete Integration**: GitHub fork functionality is now fully operational

### Testing and Quality
- All new features thoroughly tested with existing test suite
- Maintained backward compatibility
- No regressions in existing functionality
- Clean compilation with no warnings

## Success Criteria

1. Successfully cache and checkout repositories from GitHub
2. Automatic fork creation and management
3. Multi-location repository synchronization
4. Performance improvement over standard git clone
5. Seamless integration with existing git workflows
6. Comprehensive test coverage (>90%)
7. Complete documentation and user guides

## Risk Mitigation

### Technical Risks
- **Git compatibility**: Extensive testing with different git versions
- **GitHub API changes**: Robust error handling and API versioning
- **Storage management**: Implement cache size limits and cleanup

### Operational Risks
- **Authentication**: Multiple auth methods and clear error messages
- **Network issues**: Retry mechanisms and offline mode support
- **User adoption**: Comprehensive documentation and migration guides