Architecture
============

This document describes the internal architecture and design of git-cache, including the codebase structure, data flow, and key algorithms.

Overview
--------

git-cache is designed as a high-performance caching layer for Git operations that replaces traditional ``git clone`` with enhanced caching and repository management capabilities. The system follows these core principles:

* **Efficiency**: Minimize network usage and disk space through intelligent caching
* **Safety**: Robust error handling and data integrity protection  
* **Concurrency**: Safe parallel operations with file-based locking
* **Integration**: Seamless GitHub API integration for development workflows
* **Repository Management**: Automatic forking and mirror creation for development workflows

Core Architecture
-----------------

Three-Tier Repository Model
^^^^^^^^^^^^^^^^^^^^^^^^^^^

git-cache implements a sophisticated three-tier repository architecture that optimizes storage and enables efficient development workflows:

.. code-block:: text

   ┌─────────────────┐    ┌──────────────────┐    ┌────────────────────┐
   │  Cache Repo     │    │  Read-Only       │    │  Modifiable        │
   │  (Bare)         │◄──►│  Checkout        │    │  Checkout          │
   │                 │    │                  │    │                    │
   │ ~/.cache/git/   │    │ ~/github/user/   │    │ ~/github/mithro/   │
   │ github.com/     │    │ repo/            │    │ user-repo/         │
   │ user/repo       │    │                  │    │                    │
   │                 │    │ (via alternates) │    │ (via alternates)   │
   └─────────────────┘    └──────────────────┘    └────────────────────┘
            │                       │                        │
            │                       │                        │
            ▼                       ▼                        ▼
   ┌─────────────────┐    ┌──────────────────┐    ┌────────────────────┐
   │ Complete        │    │ Strategy-based   │    │ Connected to       │
   │ Repository      │    │ Clone            │    │ Forked Repository  │
   │ - All objects   │    │ - Full/Shallow   │    │ - Development      │
   │ - All refs      │    │ - Treeless       │    │ - Push/Pull        │
   │ - All history   │    │ - Blobless       │    │ - Feature branches │
   └─────────────────┘    └──────────────────┘    └────────────────────┘

Directory Structure
^^^^^^^^^^^^^^^^^^^

git-cache follows a consistent directory organization pattern:

.. code-block:: text

   ~/.cache/git/github.com/<username>/<repo>/     # Cache repository (bare)
   ~/github/<username>/<repo>/                    # Read-only repository
   ~/github/mithro/<username>-<repo>/             # Modifiable repository

**Example for repository https://github.com/torvalds/linux:**

.. code-block:: text

   ~/.cache/git/github.com/torvalds/linux/        # Cached objects and refs
   ~/github/torvalds/linux/                       # Read-only checkout
   ~/github/mithro/torvalds-linux/                # Development checkout

This structure provides:

* **Predictable Paths**: Consistent organization across all repositories
* **Namespace Separation**: Clear distinction between read-only and modifiable checkouts
* **Storage Efficiency**: Single cache serves multiple checkouts via Git alternates

**Tier 1: Cache Repository**
* Bare repository with complete history
* Shared object store for all checkouts
* Optimized for storage efficiency
* Updated via `git fetch`

**Tier 2: Read-Only Checkout**  
* Working directory for browsing/inspection
* Uses Git alternates to reference cache
* Strategy-based cloning (shallow, treeless, blobless)
* Connected to original repository

**Tier 3: Modifiable Checkout**
* Development working directory
* Uses Git alternates to reference cache
* Connected to forked repository (if available)
* Supports push operations and feature development

Data Flow
^^^^^^^^^

.. code-block:: text

   1. User Request
      git-cache clone https://github.com/user/repo.git
                     │
                     ▼
   2. URL Parsing & Repository Detection
      ┌─────────────────────────────────────┐
      │ • Parse GitHub URL                  │
      │ • Extract owner/repo                │
      │ • Determine repository type         │
      │ • Check if fork needed              │
      └─────────────────────────────────────┘
                     │
                     ▼
   3. Cache Management
      ┌─────────────────────────────────────┐
      │ • Check existing cache              │
      │ • Create cache directory            │
      │ • Acquire file locks                │
      │ • Clone or update bare repository   │
      └─────────────────────────────────────┘
                     │
                     ▼
   4. GitHub Integration (if applicable)
      ┌─────────────────────────────────────┐
      │ • Authenticate with GitHub API      │
      │ • Create fork (if needed)           │
      │ • Set privacy settings              │
      │ • Store fork URL                    │
      └─────────────────────────────────────┘
                     │
                     ▼
   5. Checkout Creation
      ┌─────────────────────────────────────┐
      │ • Create read-only checkout         │
      │ • Create modifiable checkout        │
      │ • Configure Git alternates          │
      │ • Set up remote URLs                │
      └─────────────────────────────────────┘

Codebase Structure
------------------

File Organization
^^^^^^^^^^^^^^^^^

.. code-block:: text

   git-cache/
   ├── git-cache.c          # Main application logic
   ├── git-cache.h          # Core data structures and constants
   ├── github_api.c         # GitHub API client implementation
   ├── github_api.h         # GitHub API interface
   ├── Makefile            # Build system
   ├── tests/              # Test infrastructure
   │   ├── run_cache_tests.sh
   │   ├── run_github_tests.sh
   │   └── ...
   ├── docs/               # Documentation
   │   ├── source/
   │   └── requirements.txt
   └── README.md

Core Modules
^^^^^^^^^^^^

**git-cache.c** - Main Application Logic:

.. code-block:: c

   // Core functions
   int main(int argc, char *argv[])
   int cache_clone_repository(const struct cache_options *options)
   int cache_status()
   int cache_list_repositories()
   int cache_sync_repositories()
   int cache_clean()

**github_api.c** - GitHub Integration:

.. code-block:: c

   // GitHub API client
   struct github_client* github_client_create(const char *token)
   int github_fork_repo(struct github_client *client, ...)
   int github_set_repo_private(struct github_client *client, ...)
   int github_parse_repo_url(const char *url, char **owner, char **repo)

Data Structures
^^^^^^^^^^^^^^^

**Core Configuration:**

.. code-block:: c

   struct cache_config {
       char *cache_root;           // Cache directory path
       char *checkout_root;        // Checkout directory path  
       char *github_token;         // GitHub API token
       enum clone_strategy default_strategy;
       int verbose;                // Verbose output flag
       int force;                  // Force operations flag
       int recursive_submodules;   // Submodule support flag
   };

**Repository Information:**

.. code-block:: c

   struct repo_info {
       char *original_url;         // Original repository URL
       char *fork_url;            // Forked repository URL
       char *owner;               // Repository owner
       char *name;                // Repository name
       char *cache_path;          // Cache repository path
       char *checkout_path;       // Read-only checkout path
       char *modifiable_path;     // Modifiable checkout path
       enum repo_type type;       // GitHub/Unknown
       enum clone_strategy strategy;
       int is_fork_needed;        // Whether to create fork
       char *fork_organization;   // Organization for forks
   };

**Command Options:**

.. code-block:: c

   struct cache_options {
       enum cache_operation operation;  // clone/status/clean/sync/list
       char *url;                      // Repository URL
       enum clone_strategy strategy;   // Clone strategy
       int depth;                      // Shallow clone depth
       int verbose;                    // Verbose output
       int force;                      // Force operation
       int recursive_submodules;       // Submodule support
       char *organization;             // Fork organization
       int make_private;              // Make forks private
   };

Key Algorithms
--------------

Caching Algorithm
^^^^^^^^^^^^^^^^^

The caching algorithm optimizes for both space and time efficiency:

.. code-block:: c

   int cache_clone_repository(const struct cache_options *options) {
       // 1. Parse and validate repository URL
       struct repo_info *repo = parse_repository_url(options->url);
       
       // 2. Create or update cache repository
       if (cache_repository_exists(repo->cache_path)) {
           // Update existing cache
           acquire_lock(repo->cache_path);
           git_fetch_all_refs(repo->cache_path, repo->original_url);
           release_lock(repo->cache_path);
       } else {
           // Create new cache
           git_clone_bare(repo->original_url, repo->cache_path);
       }
       
       // 3. Handle GitHub integration
       if (repo->type == REPO_TYPE_GITHUB && repo->is_fork_needed) {
           handle_github_fork(repo, config, options);
       }
       
       // 4. Create reference-based checkouts
       create_reference_checkouts(repo, config, options);
       
       return CACHE_SUCCESS;
   }

Concurrency Control
^^^^^^^^^^^^^^^^^^^

git-cache uses file-based locking for safe concurrent operations:

.. code-block:: c

   int acquire_lock(const char *repo_path) {
       char lock_path[PATH_MAX];
       snprintf(lock_path, sizeof(lock_path), "%s.lock", repo_path);
       
       // Create lock file with PID
       int lock_fd = open(lock_path, O_CREAT | O_EXCL | O_WRONLY, 0644);
       if (lock_fd == -1) {
           if (errno == EEXIST) {
               // Lock exists, check if stale
               return handle_existing_lock(lock_path);
           }
           return CACHE_ERROR_FILESYSTEM;
       }
       
       // Write PID to lock file
       pid_t pid = getpid();
       dprintf(lock_fd, "%d\n", pid);
       close(lock_fd);
       
       return CACHE_SUCCESS;
   }

**Lock Management:**
* PID-based stale lock detection
* Configurable timeout (300 seconds default)
* Automatic cleanup on process termination
* Prevents race conditions during concurrent operations

Error Recovery
^^^^^^^^^^^^^^

Comprehensive error recovery ensures data integrity:

.. code-block:: c

   int create_cache_repository_safe(const struct repo_info *repo, 
                                   const struct cache_config *config) {
       char *backup_path = NULL;
       
       // 1. Create backup if repository exists
       if (directory_exists(repo->cache_path)) {
           backup_path = create_backup(repo->cache_path);
       }
       
       // 2. Check disk space
       if (!check_disk_space(repo->cache_path, MIN_DISK_SPACE_MB)) {
           restore_from_backup(backup_path, repo->cache_path);
           return CACHE_ERROR_FILESYSTEM;
       }
       
       // 3. Attempt operation with retry
       int result = retry_network_operation(
           create_cache_repository, repo, config, MAX_RETRIES);
       
       // 4. Validate result
       if (result == CACHE_SUCCESS) {
           if (validate_git_repository(repo->cache_path, 1)) {
               remove_backup(backup_path);
               return CACHE_SUCCESS;
           }
       }
       
       // 5. Restore from backup on failure
       restore_from_backup(backup_path, repo->cache_path);
       return result;
   }

**Recovery Features:**
* Automatic backup creation before risky operations
* Disk space validation (100MB minimum)
* Network retry with exponential backoff
* Deep git repository integrity validation
* Atomic operations using temporary directories

GitHub Integration
------------------

API Client Architecture
^^^^^^^^^^^^^^^^^^^^^^^

The GitHub API client provides robust integration:

.. code-block:: c

   struct github_client {
       char *token;                // Authentication token
       char *user_agent;          // HTTP User-Agent
       int timeout;               // Request timeout
       // Internal curl handles, etc.
   };

**Key Features:**
* HTTP/HTTPS request handling via libcurl
* JSON response parsing via libjson-c
* Rate limiting awareness
* Error code translation
* Retry logic for transient failures

Fork Management
^^^^^^^^^^^^^^^

git-cache provides sophisticated fork handling that supports development workflows with automatic repository forking and mirror creation:

**Automatic Fork Creation:**
* Forks repositories to a designated organization (e.g., ``mithro-mirrors``)
* Converts forked repositories to private for security
* Creates mirrors on remote storage systems
* Configures multiple remotes for comprehensive workflow support

**Repository Setup Features:**
* Creates partial clones with configurable strategies
* Sets up fork repositories with proper privacy settings
* Establishes remote mirrors for backup and synchronization
* Configures appropriate remote URLs for development workflows

.. code-block:: c

   int handle_github_fork(struct repo_info *repo, 
                         const struct cache_config *config,
                         const struct cache_options *options) {
       // 1. Create GitHub API client
       struct github_client *client = github_client_create(config->github_token);
       
       // 2. Attempt to create fork
       struct github_repo *forked_repo;
       int result = github_fork_repo(client, repo->owner, repo->name, 
                                    repo->fork_organization, &forked_repo);
       
       // 3. Handle result
       if (result == GITHUB_SUCCESS) {
           // Store fork URL for modifiable checkout
           repo->fork_url = strdup(forked_repo->ssh_url);
           
           // Set privacy if requested
           if (options->make_private) {
               github_set_repo_private(client, forked_repo->owner, 
                                     forked_repo->name, 1);
           }
       } else if (result == GITHUB_ERROR_INVALID) {
           // Fork already exists, construct URL
           construct_fork_url(repo);
       }
       
       github_client_destroy(client);
       return CACHE_SUCCESS;
   }

**Remote Configuration:**

git-cache automatically configures multiple remotes to support comprehensive development workflows:

.. code-block:: text

   Remote Name     | Purpose                    | URL Format
   ----------------|----------------------------|----------------------------------
   origin          | Original repository       | https://github.com/user/repo.git
   mirror-github   | Mirrored repository        | git@github.com:mithro-mirrors/user-repo.git
   mirror-local    | Local storage mirror       | ssh://big-storage.k207.mithis.com/git/user-repo.git
   upstream        | Original (for forks)       | https://github.com/user/repo.git

**Example Remote Configuration:**

.. code-block:: bash

   # After git-cache clone https://github.com/torvalds/linux.git
   cd ~/github/mithro/torvalds-linux
   git remote -v
   
   origin          https://github.com/torvalds/linux.git (fetch)
   origin          git@github.com:mithro-mirrors/torvalds-linux.git (push)
   mirror-github   git@github.com:mithro-mirrors/torvalds-linux.git (fetch)
   mirror-github   git@github.com:mithro-mirrors/torvalds-linux.git (push)
   mirror-local    ssh://big-storage.k207.mithis.com/git/torvalds-linux.git (fetch)
   mirror-local    ssh://big-storage.k207.mithis.com/git/torvalds-linux.git (push)
   upstream        https://github.com/torvalds/linux.git (fetch)
   upstream        https://github.com/torvalds/linux.git (push)

This configuration enables:
* **Development**: Push to forked repository via ``origin``
* **Synchronization**: Sync with mirrors via ``mirror-*`` remotes
* **Updates**: Pull upstream changes via ``upstream`` remote
* **Backup**: Automatic mirroring to multiple locations

Performance Optimizations
-------------------------

Object Sharing
^^^^^^^^^^^^^^

Git alternates enable efficient object sharing:

.. code-block:: text

   Cache Repository Objects:
   ~/.cache/git/github.com/user/repo/objects/
   ├── 00/
   ├── 01/
   ├── ...
   └── ff/
   
   Checkout Repository References:
   ./github/user/repo/.git/objects/info/alternates
   → "/home/user/.cache/git/github.com/user/repo/objects"
   
   Shared Objects:
   - Commits, trees, blobs shared between all checkouts
   - Only working directory files differ
   - Dramatic space savings (3x+ typical reduction)

Clone Strategy Optimization
^^^^^^^^^^^^^^^^^^^^^^^^^^^

git-cache supports multiple clone strategies to optimize for different use cases and repository sizes:

.. code-block:: c

   enum clone_strategy {
       CLONE_STRATEGY_FULL,        // Complete repository
       CLONE_STRATEGY_SHALLOW,     // Limited history depth
       CLONE_STRATEGY_TREELESS,    // No tree objects (on-demand)
       CLONE_STRATEGY_BLOBLESS     // No blob objects (on-demand)
   };

**Advanced Clone Options:**

.. code-block:: bash

   # Blobless clone (fastest for large repositories)
   git clone --filter=blob:none <url>
   
   # Treeless clone (good balance of speed and functionality)
   git clone --filter=tree:0 <url>
   
   # Shallow clone (limited history)
   git clone --depth=1 <url>

**Strategy Selection Logic:**
* **Full**: Comprehensive development, offline work, complete history needed
* **Shallow**: Quick inspection, CI/CD pipelines, limited history sufficient
* **Treeless**: Large repositories, sparse workflows, on-demand tree fetching
* **Blobless**: Very large repositories, code browsing, on-demand blob fetching

**Performance Characteristics:**

.. code-block:: text

   Strategy     | Clone Time | Disk Usage | Network Usage | Use Case
   -------------|------------|------------|---------------|------------------
   Full         | Longest    | Highest    | Highest       | Complete dev work
   Blobless     | Fast       | Low        | Low           | Code browsing
   Treeless     | Faster     | Medium     | Medium        | Sparse checkouts
   Shallow      | Fastest    | Lowest     | Lowest        | Quick inspection

Network Optimization
^^^^^^^^^^^^^^^^^^^^

Intelligent network usage reduces bandwidth:

.. code-block:: c

   // Network retry with exponential backoff
   int retry_network_operation_with_progress(const char *cmd, int max_retries,
                                           const struct cache_config *config,
                                           const char *operation) {
       int delay = 1;  // Start with 1 second
       
       for (int attempt = 1; attempt <= max_retries; attempt++) {
           show_progress_indicator(operation, 1);
           int result = run_git_command(cmd, NULL);
           clear_progress_indicator();
           
           if (result == 0) return CACHE_SUCCESS;
           
           if (attempt < max_retries) {
               if (config->verbose) {
                   printf("Network operation failed with code %d, "
                          "waiting %d seconds before retry...\n", 
                          result, delay);
               }
               sleep(delay);
               delay *= 2;  // Exponential backoff
               if (delay > 16) delay = 16;  // Cap at 16 seconds
           }
       }
       
       return CACHE_ERROR_NETWORK;
   }

Security Considerations
-----------------------

Token Management
^^^^^^^^^^^^^^^^

GitHub tokens are handled securely:

* Never logged or displayed in plain text
* Passed via environment variables only
* Validated before use
* Secure memory handling (planned enhancement)

File System Security
^^^^^^^^^^^^^^^^^^^^

Repository data is protected:

.. code-block:: c

   // Safe directory creation with proper permissions
   int ensure_directory_exists(const char *path) {
       if (mkdir(path, 0755) == -1 && errno != EEXIST) {
           return CACHE_ERROR_FILESYSTEM;
       }
       
       // Verify no symlink attacks
       struct stat st;
       if (lstat(path, &st) == -1 || !S_ISDIR(st.st_mode)) {
           return CACHE_ERROR_FILESYSTEM;
       }
       
       return CACHE_SUCCESS;
   }

**Security Features:**
* Path validation against directory traversal
* Symlink attack prevention
* Proper file permissions (755 for directories, 644 for files)
* Safe temporary file handling

Extensibility
-------------

Plugin Architecture (Future)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The architecture supports future plugin development:

.. code-block:: c

   // Plugin interface (planned)
   struct cache_plugin {
       const char *name;
       int (*init)(struct cache_config *config);
       int (*clone_hook)(struct repo_info *repo);
       int (*sync_hook)(struct repo_info *repo);
       void (*cleanup)(void);
   };

**Planned Extensions:**
* GitLab API integration
* Bitbucket support
* Custom authentication providers
* Alternative storage backends
* Webhook integration

Configuration System (Planned)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Future configuration file support:

.. code-block:: ini

   [cache]
   root = /data/git-cache
   default_strategy = blobless
   
   [github]
   token = ${GITHUB_TOKEN}
   default_org = company-mirrors
   
   [plugins]
   gitlab = enabled
   webhook = enabled

Testing Architecture
--------------------

Multi-Level Testing
^^^^^^^^^^^^^^^^^^^

Comprehensive testing ensures reliability:

.. code-block:: text

   Testing Pyramid:
   
   ┌─────────────────────────┐
   │     E2E Tests           │  ← GitHub API integration
   │   (Side Effects)        │
   ├─────────────────────────┤
   │   Integration Tests     │  ← Real git operations
   │  (Network Required)     │
   ├─────────────────────────┤
   │     Unit Tests          │  ← Logic validation
   │   (No Dependencies)     │
   └─────────────────────────┘

**Test Categories:**
* **Unit Tests**: Pure logic, no external dependencies
* **Integration Tests**: Real git operations, local repositories  
* **GitHub Tests**: API integration, requires token
* **Fork Tests**: Creates real repositories (destructive)

Future Enhancements
-------------------

Planned Architecture Improvements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **Submodule Support**: Recursive caching with reference chains
2. **Configuration Files**: Persistent settings and profiles
3. **Plugin System**: Extensible provider architecture
4. **Webhook Integration**: Real-time cache updates
5. **Distributed Caching**: Shared cache servers
6. **Performance Monitoring**: Built-in metrics and profiling

Next Steps
----------

* :doc:`api/library_root` - Explore detailed API documentation
* :doc:`contributing` - Contribute to architectural improvements
* `GitHub Repository <https://github.com/mithro/git-cache>`_ - View source code