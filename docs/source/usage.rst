Usage Guide
===========

This guide covers how to use git-cache for day-to-day development workflows.

Basic Commands
--------------

Clone Operation
^^^^^^^^^^^^^^^

The primary operation is cloning repositories with intelligent caching. git-cache replaces traditional ``git clone`` with enhanced caching and repository management:

**Basic Usage:**

.. code-block:: bash

   # Basic clone with caching
   git-cache clone https://github.com/user/repo.git

   # Clone with verbose output
   git-cache clone --verbose https://github.com/user/repo.git

   # Force re-clone even if cache exists
   git-cache clone --force https://github.com/user/repo.git

**Directory Structure Created:**

After running ``git-cache clone https://github.com/torvalds/linux.git``, you get:

.. code-block:: text

   ~/.cache/git/github.com/torvalds/linux/        # Cached objects and refs
   ~/github/torvalds/linux/                       # Read-only checkout
   ~/github/mithro/torvalds-linux/                # Development checkout

**What Happens During Clone:**

1. **Cache Creation**: Creates bare repository in ``~/.cache/git/`` for object storage
2. **Read-Only Checkout**: Creates browsable repository in ``~/github/<user>/<repo>/``
3. **Development Checkout**: Creates modifiable repository in ``~/github/mithro/<user>-<repo>/``
4. **Fork Management**: Automatically forks repository to ``mithro-mirrors`` organization
5. **Remote Configuration**: Sets up multiple remotes for comprehensive workflow support

Repository Status
^^^^^^^^^^^^^^^^^

Check the status of your cache and configuration:

.. code-block:: bash

   # Show cache status and configuration
   git-cache status

Example output:

.. code-block:: text

   Git Cache Status
   ================
   
   Configuration:
     Cache root: /home/user/.cache/git
     Checkout root: /home/user/projects/github
     GitHub token: configured
     Default strategy: full
   
   Cache directory: found
   
   Cached repositories: 3

List Cached Repositories
^^^^^^^^^^^^^^^^^^^^^^^^^

View all cached repositories with detailed information:

.. code-block:: bash

   # List all cached repositories
   git-cache list

Example output:

.. code-block:: text

   Cached Repositories
   ===================
   
   user/repo
     Cache path: /home/user/.cache/git/github.com/user/repo
     Size: 150M
     Last sync: 2025-01-15 10:30:15
     Remote URL: https://github.com/user/repo.git
     Branches: 15
     Checkout: /home/user/projects/github/user/repo (full)
     Modifiable: /home/user/projects/github/mithro/user-repo (blobless)

Cache Synchronization
^^^^^^^^^^^^^^^^^^^^^

Update all cached repositories:

.. code-block:: bash

   # Sync all cached repositories
   git-cache sync

This will:

* Fetch latest changes from all remote repositories
* Update cache repositories with new commits
* Show progress for each repository
* Report success/failure status

Cache Cleanup
^^^^^^^^^^^^^

Remove cached repositories and checkouts:

.. code-block:: bash

   # Clean cache (interactive confirmation)
   git-cache clean

   # Force clean without confirmation
   git-cache clean --force

Clone Strategies
----------------

git-cache supports multiple clone strategies to optimize for different use cases:

Full Clone (Default)
^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   git-cache clone https://github.com/user/repo.git
   # or explicitly
   git-cache clone --strategy full https://github.com/user/repo.git

* **Cache**: Complete repository with all history and objects
* **Checkout**: Complete working directory
* **Best for**: Comprehensive development, history analysis
* **Storage**: Highest (but shared via alternates)

Shallow Clone
^^^^^^^^^^^^^

.. code-block:: bash

   git-cache clone --strategy shallow https://github.com/user/repo.git
   git-cache clone --strategy shallow --depth 5 https://github.com/user/repo.git

* **Cache**: Complete repository
* **Checkout**: Limited history (default depth: 1)
* **Best for**: Quick inspection, CI/CD
* **Storage**: Minimal checkout, full cache

Treeless Clone
^^^^^^^^^^^^^^

.. code-block:: bash

   git-cache clone --strategy treeless https://github.com/user/repo.git

* **Cache**: Complete repository
* **Checkout**: No tree objects (files downloaded on-demand)
* **Best for**: Large repositories, sparse workflows
* **Storage**: Medium (trees downloaded as needed)

Blobless Clone
^^^^^^^^^^^^^^

.. code-block:: bash

   git-cache clone --strategy blobless https://github.com/user/repo.git

* **Cache**: Complete repository
* **Checkout**: No blob objects (file contents downloaded on-demand)
* **Best for**: Very large repositories, browsing code structure
* **Storage**: Low (blobs downloaded as needed)

Strategy Performance Comparison
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Performance characteristics for different clone strategies:

.. code-block:: text

   Strategy     | Clone Time | Disk Usage | Network Usage | Use Case
   -------------|------------|------------|---------------|------------------
   Full         | Longest    | Highest    | Highest       | Complete dev work
   Blobless     | Fast       | Low        | Low           | Code browsing
   Treeless     | Faster     | Medium     | Medium        | Sparse checkouts
   Shallow      | Fastest    | Lowest     | Lowest        | Quick inspection

**Example: Linux Kernel Repository**

.. code-block:: text

   Strategy     | Clone Time | Disk Usage | Network Transfer
   -------------|------------|------------|------------------
   full         | 8 minutes  | 4.2 GB     | 2.8 GB
   blobless     | 45 seconds | 180 MB     | 120 MB
   treeless     | 2 minutes  | 850 MB     | 600 MB
   shallow      | 30 seconds | 90 MB      | 80 MB

**Underlying Git Commands:**

git-cache uses these Git commands for different strategies:

.. code-block:: bash

   # Full clone
   git clone https://github.com/user/repo.git
   
   # Blobless clone (fastest for large repositories)
   git clone --filter=blob:none https://github.com/user/repo.git
   
   # Treeless clone (good balance of speed and functionality)
   git clone --filter=tree:0 https://github.com/user/repo.git
   
   # Shallow clone (limited history)
   git clone --depth=1 https://github.com/user/repo.git

Submodule Support
-----------------

Handle repositories with submodules:

.. code-block:: bash

   # Clone with recursive submodule support
   git-cache clone --recursive https://github.com/user/repo.git

This will:

* Clone the main repository with caching
* Recursively clone all submodules
* Apply the same caching strategy to submodules
* Set up proper git alternates for all repositories

Repository Structure
--------------------

Understanding the Three-Tier Architecture
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

git-cache creates three types of repository copies:

1. **Cache Repository** (``~/.cache/git/github.com/user/repo``)

.. code-block:: bash

   # Bare repository with complete history
   ls ~/.cache/git/github.com/user/repo/
   # Output: branches config description HEAD hooks info objects refs

2. **Read-Only Checkout** (``./github/user/repo``)

.. code-block:: bash

   # Working directory optimized for browsing
   ls ./github/user/repo/
   # Output: README.md src/ tests/ .git/

3. **Modifiable Checkout** (``./github/mithro/user-repo``)

.. code-block:: bash

   # Development copy connected to your fork
   ls ./github/mithro/user-repo/
   # Output: README.md src/ tests/ .git/

Working with Checkouts
^^^^^^^^^^^^^^^^^^^^^^

**Read-Only Checkout** - For browsing and inspection:

.. code-block:: bash

   cd ./github/user/repo
   
   # Browse code
   ls -la
   git log --oneline -10
   git show HEAD
   
   # Cannot push (read-only)
   git push  # This will fail

**Modifiable Checkout** - For development:

.. code-block:: bash

   cd ./github/mithro/user-repo
   
   # Make changes
   echo "# My changes" >> README.md
   git add README.md
   git commit -m "Update README"
   
   # Push to your fork
   git push origin main

GitHub Integration
------------------

Automatic Fork Creation
^^^^^^^^^^^^^^^^^^^^^^^

When cloning GitHub repositories, git-cache can automatically create forks:

.. code-block:: bash

   # Set up GitHub token
   export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   
   # Clone with automatic forking
   git-cache clone https://github.com/user/repo.git
   
   # Clone with specific organization
   git-cache clone --org my-org https://github.com/user/repo.git
   
   # Make forked repositories private
   git-cache clone --private https://github.com/user/repo.git

This will:

* Create a fork in your GitHub account (or specified organization)
* Set up the modifiable checkout to use the forked repository
* Configure proper remotes for development workflow

Remote Configuration
^^^^^^^^^^^^^^^^^^^^

git-cache automatically configures multiple remotes to support comprehensive development workflows:

.. code-block:: bash

   cd ./github/mithro/user-repo
   git remote -v
   
   # Example output after cloning https://github.com/torvalds/linux.git
   origin          https://github.com/torvalds/linux.git (fetch)
   origin          git@github.com:mithro-mirrors/torvalds-linux.git (push)
   mirror-github   git@github.com:mithro-mirrors/torvalds-linux.git (fetch)
   mirror-github   git@github.com:mithro-mirrors/torvalds-linux.git (push)
   mirror-local    ssh://big-storage.k207.mithis.com/git/torvalds-linux.git (fetch)
   mirror-local    ssh://big-storage.k207.mithis.com/git/torvalds-linux.git (push)
   upstream        https://github.com/torvalds/linux.git (fetch)
   upstream        https://github.com/torvalds/linux.git (push)

**Remote Purpose:**

.. code-block:: text

   Remote Name     | Purpose                    | URL Format
   ----------------|----------------------------|----------------------------------
   origin          | Original repository       | https://github.com/user/repo.git
   mirror-github   | Mirrored repository        | git@github.com:mithro-mirrors/user-repo.git
   mirror-local    | Local storage mirror       | ssh://big-storage.k207.mithis.com/git/user-repo.git
   upstream        | Original (for forks)       | https://github.com/user/repo.git

**Development Workflow:**

.. code-block:: bash

   # Work on your changes
   git checkout -b feature-branch
   echo "New feature" >> file.txt
   git commit -am "Add new feature"
   
   # Push to your fork
   git push origin feature-branch
   
   # Sync with upstream changes
   git fetch upstream
   git checkout main
   git merge upstream/main
   
   # Push updates to mirrors
   git push mirror-github main
   git push mirror-local main
   
   # Output:
   # origin    git@github.com:mithro/user-repo.git (fetch)
   # origin    git@github.com:mithro/user-repo.git (push)

Advanced Usage
--------------

Environment Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^

Customize git-cache behavior with environment variables:

.. code-block:: bash

   # Custom cache location
   export GIT_CACHE_ROOT=/data/git-cache
   
   # Custom checkout location  
   export GIT_CHECKOUT_ROOT=/workspace/repos
   
   # GitHub API token
   export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   
   # Run git-cache with custom configuration
   git-cache clone https://github.com/user/repo.git

Concurrent Operations
^^^^^^^^^^^^^^^^^^^^^

git-cache safely handles concurrent operations:

.. code-block:: bash

   # These can run simultaneously
   git-cache clone https://github.com/user/repo1.git &
   git-cache clone https://github.com/user/repo2.git &
   git-cache sync &
   
   # Wait for all operations to complete
   wait

File-based locking prevents conflicts and ensures data integrity.

Error Recovery
^^^^^^^^^^^^^^

git-cache includes robust error recovery:

.. code-block:: bash

   # Retry failed operations
   git-cache clone https://github.com/user/large-repo.git
   
   # If interrupted, git-cache will:
   # - Detect partial downloads
   # - Resume from backup if available
   # - Validate repository integrity
   # - Retry with exponential backoff

Workflow Examples
-----------------

Daily Development Workflow
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Morning: sync all repositories
   git-cache sync
   
   # Clone a new project
   git-cache clone https://github.com/awesome/project.git
   
   # Work in modifiable checkout
   cd ./github/mithro/awesome-project
   
   # Make changes and push
   git checkout -b feature/new-feature
   echo "new feature" >> feature.txt
   git add feature.txt
   git commit -m "Add new feature"
   git push origin feature/new-feature

Contributing to Open Source
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Clone project you want to contribute to
   git-cache clone --private https://github.com/original/project.git
   
   # Work in your fork
   cd ./github/mithro/original-project
   
   # Create feature branch
   git checkout -b fix/important-bug
   
   # Make changes and test
   # ... development work ...
   
   # Push to your fork
   git push origin fix/important-bug
   
   # Create pull request via GitHub web interface

Large Repository Management
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Clone large repository with blobless strategy
   git-cache clone --strategy blobless https://github.com/large/monorepo.git
   
   # Browse code structure (fast)
   cd ./github/large/monorepo
   find . -name "*.c" | head -20
   
   # Work on specific area (downloads files on-demand)
   cd ./github/mithro/large-monorepo
   git checkout -b feature/component-x
   
   # Only files you touch are downloaded
   vim src/component-x/main.c

Troubleshooting
---------------

Common Issues
^^^^^^^^^^^^^

**"Repository already exists" Error:**

.. code-block:: bash

   # Force re-clone
   git-cache clone --force https://github.com/user/repo.git

**Network Timeout:**

.. code-block:: bash

   # git-cache automatically retries with exponential backoff
   # Check network connectivity
   curl -I https://github.com

**Disk Space Issues:**

.. code-block:: bash

   # git-cache checks disk space before operations
   # Clean old repositories
   git-cache clean
   
   # Check cache size
   git-cache list

**GitHub API Rate Limiting:**

.. code-block:: bash

   # Check rate limit status
   curl -H "Authorization: token $GITHUB_TOKEN" \
        https://api.github.com/rate_limit

Performance Tips
^^^^^^^^^^^^^^^^

* Use appropriate clone strategies for your workflow
* Regularly sync repositories to keep caches current
* Clean unused repositories to save disk space
* Use environment variables to optimize paths for your system

Next Steps
----------

* :doc:`configuration` - Advanced configuration options
* :doc:`github_integration` - Detailed GitHub setup
* :doc:`api/library_root` - API reference for developers