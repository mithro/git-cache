Testing
=======

This page covers testing git-cache functionality, from basic unit tests to comprehensive integration testing.

Test Suite Overview
--------------------

git-cache includes a comprehensive test suite designed for multiple levels of testing:

1. **Unit Tests** - No network required, fast execution
2. **Integration Tests** - Real git operations, local repositories
3. **GitHub API Tests** - Requires GitHub token, read-only operations
4. **Fork Integration Tests** - Creates real repositories (destructive)

Running Tests
-------------

Basic Test Suite
^^^^^^^^^^^^^^^^

Run the core integration tests:

.. code-block:: bash

   # Build and run basic cache tests
   make cache-test

This runs tests for:

* Help and version commands
* Basic clone operations
* Cache hit detection
* All clone strategies (full, shallow, treeless, blobless)
* Git alternates configuration
* Error handling

Expected output:

.. code-block:: text

   Starting git-cache integration tests...
   
   Running test: Help option... PASS
   Running test: Version option... PASS
   Running test: Status command... PASS
   Running test: Basic clone... PASS
   ✓ Cache directory created
   ✓ Checkout directory created
   ✓ Modifiable directory created
   
   Git Cache Test Summary:
     Total tests: 10
     Passed: 10
     Failed: 0
   All git-cache tests passed!

Individual Test Components
^^^^^^^^^^^^^^^^^^^^^^^^^^

**URL Parsing Tests**

.. code-block:: bash

   make url-test-run

Tests various GitHub URL formats and parsing logic.

**Fork Integration Tests**

.. code-block:: bash

   make fork-test-run

Tests fork URL handling logic without requiring GitHub API calls.

**Robustness Tests**

.. code-block:: bash

   make robustness-test

Tests error conditions, interruption recovery, and edge cases.

**Concurrent Operations Tests**

.. code-block:: bash

   make concurrent-test

Tests file locking and concurrent operation safety.

GitHub Integration Testing
---------------------------

Safe Testing
^^^^^^^^^^^^

Test GitHub integration without side effects:

.. code-block:: bash

   # Run safe GitHub tests (no forks created)
   ./test_github_integration_safe.sh

This performs:

* Fork logic unit tests
* GitHub URL parsing validation
* Repository detection logic
* Basic API connectivity (if token available)

Authenticated Testing
^^^^^^^^^^^^^^^^^^^^^

Test with GitHub API access:

.. code-block:: bash

   # Set up authentication
   export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   
   # Run authenticated tests
   ./test_github_integration_safe.sh

Additional tests with authentication:

* Token validation
* Repository information retrieval
* API error handling
* Rate limiting behavior

Fork Operation Testing
^^^^^^^^^^^^^^^^^^^^^^

**⚠️ Warning**: This creates real repositories!

.. code-block:: bash

   # Set up test environment
   export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   export TEST_REPO_OWNER=octocat
   export TEST_REPO_NAME=Hello-World
   export RUN_FORK_TESTS=true
   
   # Run fork tests (creates real forks!)
   ./test_github_integration_safe.sh

This will:

* Create actual forks in your GitHub account
* Test fork creation in organizations
* Test privacy setting changes
* Verify fork URL generation

**Cleanup**: Manually delete test forks after testing:

.. code-block:: bash

   # Clean up test forks (replace username with yours)
   gh repo delete username/Hello-World --confirm
   gh repo delete mithro-mirrors/octocat-Hello-World --confirm

Comprehensive Testing
^^^^^^^^^^^^^^^^^^^^^

Run all tests together:

.. code-block:: bash

   # Run complete test suite
   make test-all

Testing in Development
----------------------

Setting Up Test Environment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Install Test Dependencies**

.. code-block:: bash

   # Install GitHub CLI for testing
   # Ubuntu/Debian:
   sudo apt install gh
   
   # macOS:
   brew install gh

**Create Test Configuration**

.. code-block:: bash

   # Create test-specific environment
   cat > test-env.sh << 'EOF'
   export GIT_CACHE_ROOT=/tmp/test-cache
   export GIT_CHECKOUT_ROOT=/tmp/test-checkouts  
   export GITHUB_TOKEN=ghp_test_token_here
   export TEST_REPO_OWNER=octocat
   export TEST_REPO_NAME=Hello-World
   EOF
   
   # Source when testing
   source test-env.sh

Test Development Workflow
^^^^^^^^^^^^^^^^^^^^^^^^^^

When developing new features:

.. code-block:: bash

   # 1. Write unit tests first
   vim test_new_feature.c
   
   # 2. Add to Makefile
   vim Makefile  # Add test target
   
   # 3. Test locally
   make new-feature-test
   
   # 4. Run full test suite
   make test-all
   
   # 5. Test GitHub integration
   ./test_github_integration_safe.sh

Continuous Integration
----------------------

GitHub Actions Integration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The project includes GitHub Actions workflows:

**.github/workflows/ci.yml** - Basic CI:

.. code-block:: yaml

   name: CI
   on: [push, pull_request]
   jobs:
     test:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v3
         - name: Install dependencies
           run: sudo apt-get install -y libcurl4-openssl-dev libjson-c-dev
         - name: Build
           run: make cache
         - name: Test
           run: make cache-test

**.github/workflows/test.yml** - Extended testing:

.. code-block:: yaml

   name: Test Suite
   on: [push, pull_request]
   jobs:
     comprehensive:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v3
         - name: Install dependencies
           run: sudo apt-get install -y libcurl4-openssl-dev libjson-c-dev
         - name: Build all
           run: make all
         - name: Run all tests
           run: make test-all
         - name: GitHub integration tests
           env:
             GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
           run: |
             if [ -n "$GITHUB_TOKEN" ]; then
               ./test_github_integration_safe.sh
             fi

Local CI Testing
^^^^^^^^^^^^^^^^

Test locally before pushing:

.. code-block:: bash

   # Simulate CI environment
   docker run --rm -v $(pwd):/src -w /src ubuntu:22.04 bash -c '
     apt-get update && 
     apt-get install -y build-essential libcurl4-openssl-dev libjson-c-dev &&
     make cache &&
     make cache-test
   '

Performance Testing
-------------------

Benchmarking
^^^^^^^^^^^^

Test performance with different repository sizes:

.. code-block:: bash

   # Time basic operations
   time git-cache clone https://github.com/octocat/Hello-World.git
   time git-cache clone https://github.com/torvalds/linux.git --strategy blobless
   
   # Measure cache efficiency
   git-cache list  # Shows sizes
   
   # Compare with regular git clone
   time git clone https://github.com/octocat/Hello-World.git regular-clone

Cache Efficiency Testing
^^^^^^^^^^^^^^^^^^^^^^^^

Verify object sharing works correctly:

.. code-block:: bash

   # Clone same repository multiple times
   git-cache clone https://github.com/user/repo.git
   cd github/user/repo && git checkout -b branch1
   cd ../../github/mithro/user-repo && git checkout -b branch2
   
   # Verify alternates configuration
   cat github/user/repo/.git/objects/info/alternates
   cat github/mithro/user-repo/.git/objects/info/alternates
   
   # Check object sharing
   find .cache/git/github.com/user/repo/objects -name "*.git" | wc -l
   find github/user/repo/.git/objects -name "*.git" | wc -l

Memory and Resource Testing
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Monitor resource usage:

.. code-block:: bash

   # Monitor memory usage during large operations
   /usr/bin/time -v git-cache clone https://github.com/large/repository.git
   
   # Monitor disk usage
   du -sh .cache/git/ before clone
   git-cache clone https://github.com/user/repo.git
   du -sh .cache/git/ after clone

Test Data Management
--------------------

Test Repositories
^^^^^^^^^^^^^^^^^

Use well-known, stable repositories for testing:

**Small Repositories** (< 1MB):
* ``octocat/Hello-World`` - GitHub's official test repository
* ``octocat/Spoon-Knife`` - Another small test repository

**Medium Repositories** (1-100MB):
* ``github/gitignore`` - Collection of gitignore templates
* ``microsoft/vscode`` - Popular editor (if testing large repos)

**Large Repositories** (> 100MB):
* ``torvalds/linux`` - Linux kernel (use carefully)
* ``tensorflow/tensorflow`` - Large ML project (use with partial clone strategies)

Test Data Cleanup
^^^^^^^^^^^^^^^^^^

Regularly clean up test data:

.. code-block:: bash

   #!/bin/bash
   # cleanup-test-data.sh
   
   # Remove test cache
   rm -rf /tmp/test-cache
   rm -rf /tmp/test-checkouts
   
   # Remove any test repositories from current directory
   rm -rf github/octocat/
   rm -rf github/mithro/octocat-*
   
   # Clean git-cache test data
   git-cache clean --force

Automated Cleanup:

.. code-block:: bash

   # Add to cron for regular cleanup
   # Run daily at 2 AM
   0 2 * * * /path/to/cleanup-test-data.sh

Debugging Tests
---------------

Verbose Test Output
^^^^^^^^^^^^^^^^^^^

Enable verbose testing:

.. code-block:: bash

   # Run tests with verbose output
   V=1 make cache-test
   
   # Show detailed git-cache operations
   git-cache clone --verbose https://github.com/user/repo.git

Test Isolation
^^^^^^^^^^^^^^

Run tests in isolation:

.. code-block:: bash

   # Use temporary directories
   export GIT_CACHE_ROOT=$(mktemp -d)
   export GIT_CHECKOUT_ROOT=$(mktemp -d)
   
   # Run test
   git-cache clone https://github.com/user/repo.git
   
   # Cleanup
   rm -rf $GIT_CACHE_ROOT $GIT_CHECKOUT_ROOT

Debugging Failed Tests
^^^^^^^^^^^^^^^^^^^^^^

When tests fail:

.. code-block:: bash

   # Check test environment
   git-cache status
   
   # Verify dependencies
   ldd ./git-cache
   
   # Check network connectivity
   curl -I https://github.com
   
   # Test GitHub API directly
   curl -H "Authorization: token $GITHUB_TOKEN" \
        https://api.github.com/rate_limit

Contributing Tests
------------------

Adding New Tests
^^^^^^^^^^^^^^^^

When adding features, include tests:

1. **Unit Tests** - Test logic without external dependencies
2. **Integration Tests** - Test with real git operations
3. **Error Tests** - Test failure conditions
4. **Performance Tests** - Verify performance characteristics

Test Guidelines
^^^^^^^^^^^^^^^

**Good Test Practices**:
* Tests should be deterministic
* Clean up after themselves
* Handle network failures gracefully
* Use appropriate test repositories
* Include both positive and negative test cases

**Test Structure**:

.. code-block:: bash

   # test_feature.sh
   
   # Setup
   setup_test_environment
   
   # Test cases
   test_basic_functionality
   test_error_conditions
   test_edge_cases
   
   # Cleanup
   cleanup_test_environment
   
   # Report results
   report_test_results

Documentation Testing
^^^^^^^^^^^^^^^^^^^^^

Test documentation examples:

.. code-block:: bash

   # Verify all code examples in docs work
   # Extract code blocks and test them
   
   # Test installation instructions
   make clean && make cache
   
   # Test usage examples
   git-cache --help
   git-cache status

Next Steps
----------

* :doc:`contributing` - Contribute to test infrastructure
* :doc:`api/library_root` - API testing and validation
* `GitHub Issues <https://github.com/mithro/git-cache/issues>`_ - Report test failures