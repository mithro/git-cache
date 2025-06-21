GitHub Integration
==================

git-cache provides comprehensive integration with GitHub, including automatic fork creation, API-based repository management, and seamless development workflows.

Setup
-----

GitHub Token Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

First, create a GitHub Personal Access Token:

1. **Go to GitHub Settings**
   
   Visit https://github.com/settings/tokens

2. **Generate New Token**
   
   Click "Generate new token (classic)"

3. **Configure Token Permissions**
   
   Select the following scopes:
   
   * ``repo`` - Full control of private repositories
   * ``public_repo`` - Access to public repositories  
   * ``admin:org`` - Full control of orgs and teams (if using organizations)

4. **Set Environment Variable**

.. code-block:: bash

   export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   
   # Add to your shell profile for persistence
   echo 'export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx' >> ~/.bashrc

Token Verification
^^^^^^^^^^^^^^^^^^

Verify your token is configured correctly:

.. code-block:: bash

   # Test API access
   curl -H "Authorization: token $GITHUB_TOKEN" https://api.github.com/user
   
   # Test with git-cache
   git-cache status

Expected output should show "GitHub token: configured".

Automatic Fork Creation
-----------------------

Basic Fork Workflow
^^^^^^^^^^^^^^^^^^^

When you clone a GitHub repository, git-cache can automatically create a fork:

.. code-block:: bash

   # Clone with automatic forking
   git-cache clone https://github.com/original/project.git

This creates:

1. **Cache Repository**: Complete bare repository for efficient storage
2. **Read-only Checkout**: ``./github/original/project`` - for browsing
3. **Forked Repository**: Your fork at ``https://github.com/yourusername/project``
4. **Modifiable Checkout**: ``./github/mithro/original-project`` - connected to your fork

Organization Forks
^^^^^^^^^^^^^^^^^^^

Fork repositories into a specific organization:

.. code-block:: bash

   # Fork into organization
   git-cache clone --org mithro-mirrors https://github.com/original/project.git
   
   # Makes fork private in organization
   git-cache clone --org mithro-mirrors --private https://github.com/original/project.git

This is useful for:

* Company mirror repositories
* Team collaboration
* Backup/archival purposes

Advanced Fork Management
------------------------

Fork Naming Convention
^^^^^^^^^^^^^^^^^^^^^^

git-cache uses a consistent naming pattern for forks:

* **Original**: ``https://github.com/original/project``
* **Personal Fork**: ``https://github.com/yourusername/project``  
* **Organization Fork**: ``https://github.com/mithro-mirrors/original-project``

The modifiable checkout uses the fork URL automatically:

.. code-block:: bash

   cd ./github/mithro/original-project
   git remote -v
   # origin  git@github.com:mithro-mirrors/original-project.git (fetch)
   # origin  git@github.com:mithro-mirrors/original-project.git (push)

Privacy Settings
^^^^^^^^^^^^^^^^

Control fork visibility:

.. code-block:: bash

   # Create private fork
   git-cache clone --private https://github.com/public/repo.git
   
   # Organization + private
   git-cache clone --org company-mirrors --private https://github.com/public/repo.git

**Note**: You need appropriate permissions to make repositories private.

Fork URL Resolution
^^^^^^^^^^^^^^^^^^^

git-cache intelligently handles fork URLs:

1. **Successful Fork**: Uses the API-returned fork URL
2. **Existing Fork**: Constructs expected fork URL  
3. **Fork Failed**: Falls back to original repository URL

This ensures the modifiable checkout always points to the correct repository.

Development Workflows
---------------------

Contributing to Open Source
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Step 1: Clone and Fork**

.. code-block:: bash

   # Clone project with automatic forking
   git-cache clone https://github.com/awesome/project.git

**Step 2: Create Feature Branch**

.. code-block:: bash

   # Work in your fork's checkout
   cd ./github/mithro/awesome-project
   
   # Create feature branch
   git checkout -b feature/amazing-improvement
   
   # Make changes
   echo "Amazing improvement" >> README.md
   git add README.md
   git commit -m "Add amazing improvement"

**Step 3: Push and Create PR**

.. code-block:: bash

   # Push to your fork
   git push origin feature/amazing-improvement
   
   # Create pull request via GitHub CLI (optional)
   gh pr create --title "Amazing improvement" --body "This adds an amazing improvement"

Corporate Development
^^^^^^^^^^^^^^^^^^^^^

**Setup Organization Mirroring**

.. code-block:: bash

   # Set default organization
   export GITHUB_ORG=company-mirrors
   
   # Clone external projects into company mirrors
   git-cache clone --org company-mirrors --private https://github.com/external/library.git

**Benefits:**
* Compliance with corporate policies
* Backup of external dependencies
* Internal modification tracking
* Security scanning integration

Multiple Repository Management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Bulk Operations**

.. code-block:: bash

   # Clone multiple related repositories
   for repo in repo1 repo2 repo3; do
       git-cache clone --org company-mirrors https://github.com/upstream/$repo.git
   done
   
   # Sync all repositories daily
   git-cache sync

API Integration
---------------

Repository Information
^^^^^^^^^^^^^^^^^^^^^^

git-cache can retrieve detailed repository information:

.. code-block:: bash

   # The GitHub API client provides:
   # - Repository metadata
   # - Fork relationships  
   # - Branch information
   # - Access permissions

This information is used for:

* Intelligent fork detection
* Repository validation
* Cache optimization decisions

Error Handling
^^^^^^^^^^^^^^

git-cache gracefully handles GitHub API errors:

**Rate Limiting**

.. code-block:: text

   Warning: GitHub API rate limit approached
   Continuing with reduced functionality...

**Authentication Issues**

.. code-block:: text

   Error: GitHub token invalid or expired
   Please check your GITHUB_TOKEN environment variable

**Repository Access**

.. code-block:: text

   Warning: Cannot access private repository
   Cloning as public repository without fork

**Network Issues**

.. code-block:: text

   Warning: GitHub API unavailable
   Continuing with local-only operations...

Testing GitHub Integration
--------------------------

Unit Tests
^^^^^^^^^^

Test fork logic without side effects:

.. code-block:: bash

   # Run fork integration tests (no API calls)
   make fork-test-run

Integration Tests
^^^^^^^^^^^^^^^^^

Test with real GitHub API:

.. code-block:: bash

   # Set up test environment
   export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   export TEST_REPO_OWNER=octocat
   export TEST_REPO_NAME=Hello-World
   
   # Run GitHub integration tests
   ./test_github_integration_safe.sh

**Safe Testing Strategy:**

1. **Unit tests** run locally without network
2. **Integration tests** use read-only API calls
3. **Fork tests** require explicit consent (creates real forks)

Security Considerations
-----------------------

Token Security
^^^^^^^^^^^^^^

**Best Practices:**

.. code-block:: bash

   # Use minimal token permissions
   # For public repositories only:
   # - public_repo
   
   # For private repositories:
   # - repo
   
   # For organization management:
   # - admin:org (use sparingly)

**Token Storage:**

.. code-block:: bash

   # Store securely
   chmod 600 ~/.github-token
   echo "ghp_token_here" > ~/.github-token
   export GITHUB_TOKEN=$(cat ~/.github-token)
   
   # Or use credential managers
   # macOS Keychain, Linux gnome-keyring, etc.

Repository Privacy
^^^^^^^^^^^^^^^^^^

**Automatic Privacy:**

.. code-block:: bash

   # Always make forks private for sensitive code
   git-cache clone --private https://github.com/company/internal-project.git

**Access Control:**

* Review fork permissions regularly
* Use organization forks for team projects
* Monitor repository access logs

Troubleshooting
---------------

Common Issues
^^^^^^^^^^^^^

**"Fork already exists" Error**

This is not actually an error - git-cache handles existing forks gracefully:

.. code-block:: text

   Fork already exists or validation error
   Using constructed fork URL: git@github.com:username/repo.git

**API Rate Limiting**

.. code-block:: bash

   # Check current rate limit
   curl -H "Authorization: token $GITHUB_TOKEN" \
        https://api.github.com/rate_limit
   
   # Output shows remaining requests
   {
     "resources": {
       "core": {
         "limit": 5000,
         "remaining": 4999,
         "reset": 1640995200
       }
     }
   }

**Fork Permission Issues**

.. code-block:: text

   Error: Cannot fork to organization 'company'
   Permission denied or organization not found

Solutions:

1. Check organization membership
2. Verify token has ``admin:org`` permission
3. Use personal forks instead: remove ``--org`` flag

**Network Connectivity**

.. code-block:: bash

   # Test GitHub connectivity
   curl -I https://api.github.com
   
   # Test with specific token
   curl -H "Authorization: token $GITHUB_TOKEN" \
        https://api.github.com/user

Advanced Configuration
----------------------

Custom Fork Naming
^^^^^^^^^^^^^^^^^^^

While not configurable via settings yet, you can understand the naming logic:

.. code-block:: c

   // From the source code:
   // Organization fork: "org/original-repo"  
   // Personal fork: "username/repo"

Future enhancements may allow custom naming patterns.

Webhook Integration
^^^^^^^^^^^^^^^^^^^

Future versions may support webhook integration for:

* Automatic cache updates when repositories change
* Notification of new releases
* Sync triggering from GitHub events

API Client Customization
^^^^^^^^^^^^^^^^^^^^^^^^^

The GitHub API client supports:

* Custom timeout settings
* Retry logic with exponential backoff
* User-agent customization
* Debug logging

Example Workflows
-----------------

Daily Development
^^^^^^^^^^^^^^^^^

.. code-block:: bash

   #!/bin/bash
   # daily-dev-setup.sh
   
   # Sync all cached repositories
   git-cache sync
   
   # Clone today's work projects
   git-cache clone --org company-mirrors https://github.com/client/project.git
   git-cache clone https://github.com/opensource/library.git
   
   # Show status
   git-cache list

Release Preparation
^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   #!/bin/bash
   # prepare-release.sh
   
   # Ensure all dependencies are cached and up-to-date
   git-cache sync
   
   # Clone release repositories
   for repo in main-app frontend backend; do
       git-cache clone --org company-releases https://github.com/company/$repo.git
   done
   
   # Verify all repositories are ready
   git-cache status

Next Steps
----------

* :doc:`api/library_root` - Use git-cache programmatically
* :doc:`contributing` - Contribute to GitHub integration features
* :doc:`testing` - Set up comprehensive testing