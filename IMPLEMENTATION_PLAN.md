# Git Cache Tool Implementation Plan

## Overview

This document outlines the implementation plan for a custom git clone replacement with advanced caching and mirroring capabilities. The tool will be implemented as a git subcommand called `git-cache` that provides intelligent repository caching, automatic forking, and multi-location mirroring.

## Project Goals

- Replace standard `git clone` with a caching-aware alternative
- Implement intelligent repository mirroring across multiple locations
- Support shallow/treeless/blobless repository optimizations
- Provide seamless integration with existing git workflows

## Architecture Overview

### Repository Structure

The tool will manage repositories in three distinct locations:

1. **Cache Repository**: `~/.cache/git/github.com/<username>/<repo>`
   - **Full bare repository** containing ALL git objects (blobs, trees, commits)
   - Acts as complete local object store and reference mirror
   - Contains full history and all file contents
   - Shared across multiple checkouts for storage efficiency

2. **Read-Only Checkout**: `~/github/<username>/<repo>`
   - **Shallow/treeless/blobless** working directory checkout
   - References cache repository using `--reference` mechanism
   - Optimized for browsing and inspection with minimal local storage
   - Git objects shared with cache repository

3. **Modifiable Checkout**: `~/github/mithro/<username>-<repo>`
   - **Partial clone** working directory for development
   - References cache repository using `--reference` mechanism
   - Connected to forked repository for push operations
   - Supports development workflows with shared object storage

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
- [ ] Extend argument parsing for cache-specific options

#### 1.2 Cache Directory Management
- [ ] Create `~/.cache/git` directory structure
- [ ] Implement cache location resolution
- [ ] Add cache cleanup and maintenance functions
- [ ] Design cache metadata storage

#### 1.3 URL Parsing and Repository Identification
- [ ] Parse GitHub URLs (https/ssh formats)
- [ ] Extract username/repository information
- [ ] Validate repository accessibility
- [ ] Handle various git URL formats

### Phase 2: Caching Implementation

#### 2.1 Full Cache Repository Management
- [ ] Create **full bare repository** in cache location
- [ ] Implement complete git clone to populate cache with ALL objects
- [ ] Fetch all branches, tags, and references
- [ ] Maintain complete repository history and blobs
- [ ] Support incremental updates to keep cache current

#### 2.2 Reference-Based Checkout Creation
- [ ] Create partial checkouts using `--reference` to cache repository
- [ ] Implement checkout strategies:
  - Shallow checkouts (`--depth=1 --reference=<cache>`)
  - Treeless checkouts (`--filter=tree:0 --reference=<cache>`)
  - Blobless checkouts (`--filter=blob:none --reference=<cache>`)
- [ ] Configure git alternates automatically
- [ ] Handle reference repository validation

#### 2.3 Object Sharing and Storage Optimization
- [ ] Implement git alternates configuration
- [ ] Verify object sharing between cache and checkouts
- [ ] Handle cache repository updates and checkout synchronization
- [ ] Implement storage deduplication verification
- [ ] Add checkout repair mechanisms when cache is updated

### Phase 3: GitHub Integration

#### 3.1 GitHub API Integration
- [ ] Implement GitHub API client
- [ ] Handle authentication (tokens, SSH keys)
- [ ] Repository existence checking
- [ ] Fork creation and management

#### 3.2 Fork Management
- [ ] Create fork in mithro-mirrors organization
- [ ] Set fork privacy settings
- [ ] Configure fork repository settings
- [ ] Handle fork naming conflicts

#### 3.3 Remote Mirror Setup
- [ ] Configure remote mirrors
- [ ] Set up push/pull relationships
- [ ] Handle authentication for multiple remotes
- [ ] Implement mirror synchronization

### Phase 4: Git Submodule Support

#### 4.1 Submodule Detection and Analysis
- [ ] Parse `.gitmodules` file in repositories
- [ ] Detect submodule URLs and paths
- [ ] Identify submodule commit SHAs
- [ ] Handle relative submodule URLs
- [ ] Support nested submodules (recursive detection)

#### 4.2 Submodule Cache Management
- [ ] Create cache entries for each submodule
- [ ] Implement recursive caching: `~/.cache/git/github.com/<submodule-user>/<submodule-repo>`
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
- [ ] `git cache status` - Show cache status (including submodules)
- [ ] `git cache clean` - Clean cache (with submodule support)
- [ ] `git cache sync` - Force synchronization (recursive)
- [ ] `git cache list` - List cached repositories and submodules
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

- `~/.cache/git/config` - Global cache configuration
- `~/.config/git-cache/settings` - User preferences  
- Per-repository metadata in cache directories
- `~/.cache/git/github.com/<user>/<repo>/submodules.cache` - Submodule cache mapping

### Reference Architecture Example

```bash
# Step 1: Create full bare repository in cache
git clone --bare https://github.com/user/repo.git ~/.cache/git/github.com/user/repo

# Step 2: Create shallow checkout using reference
git clone --depth=1 --reference ~/.cache/git/github.com/user/repo \
    https://github.com/user/repo.git ~/github/user/repo

# Step 3: Create development checkout with reference
git clone --filter=blob:none --reference ~/.cache/git/github.com/user/repo \
    https://github.com/mithro-mirrors/user-repo.git ~/github/mithro/user-repo

# Result: Three repositories sharing the same git objects
# - Cache: ~2GB (full repository)
# - Read-only: ~50MB (shallow + references)
# - Development: ~100MB (treeless + references)
# Total storage: ~2.15GB instead of ~6GB
```

### Submodule Reference Chain Example

```bash
# Parent repository with submodules
~/.cache/git/github.com/user/parent-repo/           # Full parent cache
~/.cache/git/github.com/vendor/submodule1/          # Full submodule1 cache  
~/.cache/git/github.com/vendor/submodule2/          # Full submodule2 cache

# Checkout with submodule references
~/github/user/parent-repo/                          # Shallow parent checkout
~/github/user/parent-repo/vendor/submodule1/        # References submodule1 cache
~/github/user/parent-repo/vendor/submodule2/        # References submodule2 cache

# Git alternates chain:
# parent-repo/.git/objects/info/alternates -> ~/.cache/git/github.com/user/parent-repo/objects
# submodule1/.git/objects/info/alternates -> ~/.cache/git/github.com/vendor/submodule1/objects
# submodule2/.git/objects/info/alternates -> ~/.cache/git/github.com/vendor/submodule2/objects
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
- User-local installation (`~/.local/bin`)
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