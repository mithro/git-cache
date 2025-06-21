Configuration
=============

This page covers how to configure git-cache for your specific workflow and environment.

Environment Variables
----------------------

git-cache uses environment variables for configuration. These can be set in your shell profile or passed directly to commands.

Core Configuration
^^^^^^^^^^^^^^^^^^

GIT_CACHE_ROOT
""""""""""""""

Controls where cached bare repositories are stored.

.. code-block:: bash

   # Default: ~/.cache/git
   export GIT_CACHE_ROOT=/data/git-cache
   
   # Verify setting
   git-cache status

**Use cases:**
* Large cache on separate disk
* Shared cache for team environments
* Network-attached storage for distributed teams

GIT_CHECKOUT_ROOT  
"""""""""""""""""

Controls where working directory checkouts are created.

.. code-block:: bash

   # Default: ./github (relative to current directory)
   export GIT_CHECKOUT_ROOT=/workspace/projects
   
   # Clone will create: /workspace/projects/user/repo
   git-cache clone https://github.com/user/repo.git

**Use cases:**
* Centralized project directory
* Separate disk for working files
* Integration with IDE workspace

GitHub Integration
^^^^^^^^^^^^^^^^^^

GITHUB_TOKEN
""""""""""""

GitHub Personal Access Token for API operations.

.. code-block:: bash

   # Generate token at: https://github.com/settings/tokens
   export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

**Required permissions:**
* ``repo`` - For private repository access
* ``public_repo`` - For public repository forking
* ``admin:org`` - For organization fork management (if using organizations)

**Generate token:**

1. Go to https://github.com/settings/tokens
2. Click "Generate new token (classic)"
3. Select required scopes
4. Copy token and set environment variable

Configuration Files
-------------------

Future Enhancement
^^^^^^^^^^^^^^^^^^

Configuration file support is planned for a future release:

.. code-block:: ini

   # ~/.gitcacherc (planned feature)
   [cache]
   root = /data/git-cache
   checkout_root = /workspace/projects
   default_strategy = blobless
   
   [github]
   token = ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   default_org = mithro-mirrors
   make_private = true
   
   [sync]
   auto_sync = daily
   parallel_jobs = 4

Until then, use environment variables or shell scripts for configuration.

Per-Project Configuration
-------------------------

Directory-specific Settings
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can set different configurations for different projects:

.. code-block:: bash

   # Project A - use specific cache location
   cd /workspace/project-a
   export GIT_CACHE_ROOT=/fast-ssd/cache
   git-cache clone https://github.com/project-a/repo.git
   
   # Project B - use network storage
   cd /workspace/project-b  
   export GIT_CACHE_ROOT=/network/shared-cache
   git-cache clone https://github.com/project-b/repo.git

Shell Integration
^^^^^^^^^^^^^^^^^

Create project-specific configuration scripts:

.. code-block:: bash

   # ~/.config/git-cache/project-a.sh
   export GIT_CACHE_ROOT=/fast-ssd/cache
   export GIT_CHECKOUT_ROOT=/workspace/project-a
   export GITHUB_TOKEN=ghp_project_specific_token

   # Source before working on project A
   source ~/.config/git-cache/project-a.sh
   git-cache clone https://github.com/project-a/repo.git

Advanced Configuration
----------------------

Clone Strategy Defaults
^^^^^^^^^^^^^^^^^^^^^^^^

While there's no configuration file yet, you can create wrapper scripts:

.. code-block:: bash

   # ~/.local/bin/git-cache-blobless
   #!/bin/bash
   exec git-cache clone --strategy blobless "$@"
   
   # ~/.local/bin/git-cache-org
   #!/bin/bash
   exec git-cache clone --org mithro-mirrors --private "$@"

Make them executable:

.. code-block:: bash

   chmod +x ~/.local/bin/git-cache-*

Performance Tuning
^^^^^^^^^^^^^^^^^^^

**Large Repository Optimization:**

.. code-block:: bash

   # Use SSD for cache
   export GIT_CACHE_ROOT=/fast-ssd/git-cache
   
   # Use HDD for checkouts (less I/O intensive)
   export GIT_CHECKOUT_ROOT=/data/checkouts

**Network Optimization:**

.. code-block:: bash

   # For slow networks, prefer shallow clones
   alias git-cache-fast='git-cache clone --strategy shallow --depth 1'
   
   # For fast networks with large repos, use blobless
   alias git-cache-large='git-cache clone --strategy blobless'

Team/Organization Setup
-----------------------

Shared Cache Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^

For teams sharing a cache server:

.. code-block:: bash

   # Team shared configuration
   export GIT_CACHE_ROOT=/shared/git-cache
   export GIT_CHECKOUT_ROOT=$HOME/projects
   
   # Ensure proper permissions
   sudo mkdir -p /shared/git-cache
   sudo chgrp developers /shared/git-cache
   sudo chmod g+w /shared/git-cache

Organization Fork Management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For organizations with dedicated mirror accounts:

.. code-block:: bash

   # Use organization for all forks
   export GITHUB_ORG=company-mirrors
   
   # Wrapper script for organization cloning
   #!/bin/bash
   # ~/.local/bin/company-clone
   git-cache clone --org company-mirrors --private "$@"

Container/CI Configuration
--------------------------

Docker Environment
^^^^^^^^^^^^^^^^^^

.. code-block:: dockerfile

   # Dockerfile
   FROM ubuntu:22.04
   
   # Install dependencies
   RUN apt-get update && apt-get install -y \
       build-essential git libcurl4-openssl-dev libjson-c-dev
   
   # Build git-cache
   COPY . /src/git-cache
   WORKDIR /src/git-cache
   RUN make cache && make install
   
   # Configure for CI
   ENV GIT_CACHE_ROOT=/cache
   ENV GIT_CHECKOUT_ROOT=/workspace
   ENV GITHUB_TOKEN=""
   
   # Create cache directory
   RUN mkdir -p /cache /workspace
   
   WORKDIR /workspace
   ENTRYPOINT ["git-cache"]

GitHub Actions
^^^^^^^^^^^^^^

.. code-block:: yaml

   # .github/workflows/test.yml
   name: Test with git-cache
   
   on: [push, pull_request]
   
   jobs:
     test:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v3
         
         - name: Install dependencies
           run: |
             sudo apt-get update
             sudo apt-get install -y libcurl4-openssl-dev libjson-c-dev
         
         - name: Build git-cache
           run: |
             make cache
             sudo make install
         
         - name: Configure git-cache
           env:
             GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
           run: |
             export GIT_CACHE_ROOT=$GITHUB_WORKSPACE/.cache
             export GIT_CHECKOUT_ROOT=$GITHUB_WORKSPACE/repos
             git-cache status
         
         - name: Test repository cloning
           run: |
             git-cache clone https://github.com/octocat/Hello-World.git
             git-cache list

Security Considerations
-----------------------

Token Management
^^^^^^^^^^^^^^^^

**Store tokens securely:**

.. code-block:: bash

   # Use a credential manager
   echo "export GITHUB_TOKEN=$(security find-generic-password -w -s github-token)" >> ~/.bashrc
   
   # Or use a dedicated secrets file
   echo "ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" > ~/.github-token
   chmod 600 ~/.github-token
   export GITHUB_TOKEN=$(cat ~/.github-token)

**Limit token permissions:**

* Use fine-grained personal access tokens when available
* Only grant necessary repository permissions
* Regularly rotate tokens
* Use different tokens for different projects/environments

Cache Security
^^^^^^^^^^^^^^

**Protect cache directories:**

.. code-block:: bash

   # Ensure proper permissions
   chmod 700 ~/.cache/git
   
   # For shared caches, use groups
   sudo chgrp developers /shared/git-cache
   sudo chmod 750 /shared/git-cache

**Regular cleanup:**

.. code-block:: bash

   # Clean unused repositories monthly
   git-cache clean
   
   # Verify cache integrity
   git-cache status

Troubleshooting Configuration
-----------------------------

Debug Configuration
^^^^^^^^^^^^^^^^^^^

Check current configuration:

.. code-block:: bash

   # Show all configuration
   git-cache status
   
   # Show environment variables
   env | grep GIT
   env | grep GITHUB

**Common configuration issues:**

**Path Problems:**

.. code-block:: bash

   # Check if paths exist and are writable
   ls -la $GIT_CACHE_ROOT
   touch $GIT_CACHE_ROOT/test && rm $GIT_CACHE_ROOT/test

**Token Issues:**

.. code-block:: bash

   # Test token validity
   curl -H "Authorization: token $GITHUB_TOKEN" \
        https://api.github.com/user

**Permission Problems:**

.. code-block:: bash

   # Check directory permissions
   namei -l $GIT_CACHE_ROOT
   
   # Fix permissions
   sudo chown -R $USER:$USER $GIT_CACHE_ROOT

Validation Scripts
^^^^^^^^^^^^^^^^^^

Create a configuration validation script:

.. code-block:: bash

   #!/bin/bash
   # validate-config.sh
   
   echo "=== git-cache Configuration Validation ==="
   
   # Check environment variables
   echo "Cache root: ${GIT_CACHE_ROOT:-default}"
   echo "Checkout root: ${GIT_CHECKOUT_ROOT:-default}"
   echo "GitHub token: ${GITHUB_TOKEN:+configured}"
   
   # Check directories
   if [ -n "$GIT_CACHE_ROOT" ]; then
       if [ -d "$GIT_CACHE_ROOT" ] && [ -w "$GIT_CACHE_ROOT" ]; then
           echo "✓ Cache directory accessible"
       else
           echo "✗ Cache directory not accessible"
       fi
   fi
   
   # Test GitHub API
   if [ -n "$GITHUB_TOKEN" ]; then
       if curl -s -H "Authorization: token $GITHUB_TOKEN" \
               https://api.github.com/user >/dev/null; then
           echo "✓ GitHub token valid"
       else
           echo "✗ GitHub token invalid"
       fi
   fi
   
   # Test git-cache
   if command -v git-cache >/dev/null; then
       echo "✓ git-cache installed"
       git-cache --version
   else
       echo "✗ git-cache not found"
   fi

Best Practices
--------------

1. **Use dedicated SSDs for cache** - Improves performance significantly
2. **Regular maintenance** - Run ``git-cache sync`` and ``git-cache clean`` regularly  
3. **Monitor disk usage** - Use ``git-cache list`` to track cache sizes
4. **Secure tokens** - Use minimal permissions and rotate regularly
5. **Team coordination** - Establish shared cache policies for organizations
6. **Backup important caches** - Consider backing up valuable cache data

Next Steps
----------

* :doc:`github_integration` - Set up advanced GitHub workflows
* :doc:`api/library_root` - Programmatic integration
* :doc:`contributing` - Contribute to git-cache development