Contributing
============

We welcome contributions to git-cache! This guide covers how to contribute code, documentation, and tests.

Getting Started
---------------

Development Setup
^^^^^^^^^^^^^^^^^

1. **Fork and Clone**

.. code-block:: bash

   # Fork on GitHub, then clone your fork
   git clone https://github.com/yourusername/git-cache.git
   cd git-cache

2. **Install Dependencies**

.. code-block:: bash

   # Ubuntu/Debian
   sudo apt-get install build-essential git libcurl4-openssl-dev libjson-c-dev \
                        valgrind cppcheck clang-format doxygen
   
   # Documentation dependencies
   pip install -r docs/requirements.txt

3. **Build and Test**

.. code-block:: bash

   # Build everything
   make all
   
   # Run tests
   make test-all
   
   # Build documentation
   cd docs && make html

4. **Set Up Development Environment**

.. code-block:: bash

   # Set up git hooks (optional)
   cp scripts/pre-commit .git/hooks/pre-commit
   chmod +x .git/hooks/pre-commit
   
   # Configure git
   git config user.name "Your Name"
   git config user.email "your.email@example.com"

Types of Contributions
----------------------

Code Contributions
^^^^^^^^^^^^^^^^^^

**Bug Fixes**
* Fix reported issues
* Add regression tests
* Update documentation if needed

**New Features**
* Implement features from the roadmap
* Add comprehensive tests
* Update documentation
* Consider backward compatibility

**Performance Improvements**  
* Optimize algorithms
* Reduce memory usage
* Improve I/O efficiency
* Add benchmarks

**Platform Support**
* Support new operating systems
* Fix platform-specific issues
* Improve build system

Documentation Contributions
^^^^^^^^^^^^^^^^^^^^^^^^^^^

**User Documentation**
* Improve existing guides
* Add new usage examples
* Fix typos and clarity issues
* Translate documentation

**API Documentation**
* Document functions and data structures
* Add code examples
* Improve Doxygen comments
* Generate comprehensive API reference

**Developer Documentation**
* Architectural overviews
* Design decisions
* Implementation guides
* Testing strategies

Testing Contributions
^^^^^^^^^^^^^^^^^^^^^

**Test Coverage**
* Add unit tests for uncovered code
* Create integration tests
* Add performance benchmarks
* Improve test infrastructure

**Testing Infrastructure**
* Enhance build system
* Add new test frameworks
* Improve CI/CD pipelines
* Create testing utilities

Development Workflow
--------------------

Creating Changes
^^^^^^^^^^^^^^^^

1. **Create Feature Branch**

.. code-block:: bash

   # Create and switch to feature branch
   git checkout -b feature/awesome-improvement
   
   # Or for bug fixes
   git checkout -b fix/issue-123

2. **Make Changes**

.. code-block:: bash

   # Edit files
   vim git-cache.c
   
   # Test changes
   make cache
   ./git-cache --help

3. **Follow Coding Standards**

**C Code Style:**
* Use Git project coding style (tabs for indentation)
* Keep lines under 80 characters when reasonable
* Use clear, descriptive variable names
* Add comments for complex logic

**Example:**

.. code-block:: c

   /* Good: Clear function with proper formatting */
   static int create_cache_directory(const char *path)
   {
       if (!path) {
           return CACHE_ERROR_ARGS;
       }
       
       /* Create directory with proper permissions */
       if (mkdir(path, 0755) == -1 && errno != EEXIST) {
           return CACHE_ERROR_FILESYSTEM;
       }
       
       return CACHE_SUCCESS;
   }

4. **Add Tests**

.. code-block:: bash

   # Add unit tests
   vim test_new_feature.c
   
   # Update Makefile if needed
   vim Makefile
   
   # Test your changes
   make test-all

5. **Update Documentation**

.. code-block:: bash

   # Update user documentation
   vim docs/source/usage.rst
   
   # Add API documentation
   vim git-cache.h  # Add Doxygen comments
   
   # Build and verify docs
   cd docs && make html

Code Review Process
^^^^^^^^^^^^^^^^^^^

**Before Submitting:**

.. code-block:: bash

   # Run full test suite
   make test-all
   
   # Check code style
   clang-format -i *.c *.h
   
   # Run static analysis
   cppcheck --enable=warning *.c
   
   # Build documentation
   cd docs && make html

**Commit Guidelines:**

.. code-block:: bash

   # Good commit message format
   git commit -m "Add feature: GitHub organization support

   Implements automatic fork creation in specified organizations:
   - Add --org flag for organization selection
   - Update GitHub API client for org operations
   - Add comprehensive tests and documentation
   - Maintain backward compatibility

   Fixes #123"

**Submitting Pull Request:**

1. Push to your fork
2. Create pull request on GitHub
3. Fill out PR template
4. Respond to review feedback
5. Update documentation if requested

Testing Requirements
--------------------

Test Coverage Standards
^^^^^^^^^^^^^^^^^^^^^^^

All contributions must include appropriate tests:

**Unit Tests** (Required)
* Test new functions in isolation
* Cover error conditions
* Use descriptive test names
* No external dependencies

**Integration Tests** (Required for features)
* Test end-to-end functionality
* Use real git operations
* Verify expected behavior
* Clean up test data

**GitHub Tests** (Required for GitHub features)
* Mock API responses when possible
* Use safe test repositories
* Handle authentication gracefully
* Document any side effects

Example Test Structure
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   /* test_new_feature.c */
   #include <assert.h>
   #include <stdio.h>
   #include "git-cache.h"

   static void test_basic_functionality(void)
   {
       printf("Testing basic functionality...\n");
       
       /* Test normal case */
       int result = new_feature_function("valid_input");
       assert(result == CACHE_SUCCESS);
       
       /* Test error case */
       result = new_feature_function(NULL);
       assert(result == CACHE_ERROR_ARGS);
       
       printf("✓ Basic functionality tests passed\n");
   }

   static void test_edge_cases(void)
   {
       printf("Testing edge cases...\n");
       
       /* Test edge conditions */
       int result = new_feature_function("");
       assert(result == CACHE_ERROR_ARGS);
       
       printf("✓ Edge case tests passed\n");
   }

   int main(void)
   {
       printf("New Feature Test Suite\n");
       printf("======================\n\n");
       
       test_basic_functionality();
       test_edge_cases();
       
       printf("\nAll tests passed!\n");
       return 0;
   }

Documentation Standards
-----------------------

Code Documentation
^^^^^^^^^^^^^^^^^^

**Function Documentation:**

.. code-block:: c

   /**
    * @brief Create a new cache repository
    * 
    * Creates a bare Git repository for caching purposes. The repository
    * will be used as a shared object store for multiple checkouts.
    * 
    * @param repo_url The URL of the repository to cache
    * @param cache_path The local path where the cache will be created
    * @param config Cache configuration settings
    * 
    * @return CACHE_SUCCESS on success, or appropriate error code
    * 
    * @note This function requires network access to clone the repository
    * @see cache_update_repository() for updating existing caches
    * 
    * @example
    * @code
    * struct cache_config config = {0};
    * int result = create_cache_repository(
    *     "https://github.com/user/repo.git",
    *     "/home/user/.cache/git/repo",
    *     &config
    * );
    * if (result != CACHE_SUCCESS) {
    *     fprintf(stderr, "Cache creation failed\n");
    * }
    * @endcode
    */
   int create_cache_repository(const char *repo_url, 
                              const char *cache_path,
                              const struct cache_config *config);

**Data Structure Documentation:**

.. code-block:: c

   /**
    * @brief Repository information and metadata
    * 
    * Contains all information needed to manage a cached repository,
    * including paths, URLs, and configuration settings.
    */
   struct repo_info {
       /** @brief Original repository URL */
       char *original_url;
       
       /** @brief Fork repository URL (if created) */
       char *fork_url;
       
       /** @brief Repository owner (from URL parsing) */
       char *owner;
       
       /** @brief Repository name (from URL parsing) */
       char *name;
       
       /** @brief Path to cached bare repository */
       char *cache_path;
       
       /** @brief Path to read-only checkout */
       char *checkout_path;
       
       /** @brief Path to modifiable checkout */
       char *modifiable_path;
       
       /** @brief Repository type (GitHub, etc.) */
       enum repo_type type;
       
       /** @brief Clone strategy for checkouts */
       enum clone_strategy strategy;
       
       /** @brief Whether automatic forking is needed */
       int is_fork_needed;
       
       /** @brief Organization for fork creation */
       char *fork_organization;
   };

User Documentation
^^^^^^^^^^^^^^^^^^

**Writing Guidelines:**
* Use clear, concise language
* Include practical examples
* Explain both what and why
* Consider different user skill levels
* Test all code examples

**Documentation Structure:**
* Overview and introduction
* Step-by-step instructions
* Code examples with expected output
* Common issues and troubleshooting
* References to related topics

Contribution Areas
------------------

Priority Areas
^^^^^^^^^^^^^^

**High Priority:**
* Submodule support implementation
* Configuration file system
* Performance optimizations
* Cross-platform compatibility
* Security enhancements

**Medium Priority:**
* Additional git hosting providers (GitLab, Bitbucket)
* Plugin architecture
* Advanced caching strategies
* Webhook integration
* Monitoring and metrics

**Documentation Needs:**
* More usage examples
* Video tutorials
* Migration guides
* Best practices guide
* Troubleshooting cookbook

Getting Help
------------

Communication Channels
^^^^^^^^^^^^^^^^^^^^^^

* **GitHub Issues**: Bug reports and feature requests
* **GitHub Discussions**: Questions and general discussion
* **Pull Request Reviews**: Code review and feedback

**Before Asking for Help:**

1. Search existing issues and documentation
2. Try to reproduce the problem
3. Gather relevant information (OS, version, logs)
4. Create minimal test case

**When Reporting Issues:**

.. code-block:: text

   Title: Brief description of the issue

   **Environment:**
   - OS: Ubuntu 22.04
   - git-cache version: 1.0.0
   - Git version: 2.34.1

   **Steps to Reproduce:**
   1. Run `git-cache clone https://github.com/user/repo.git`
   2. Observe error message

   **Expected Behavior:**
   Repository should be cloned successfully

   **Actual Behavior:**
   Error: Network timeout after 30 seconds

   **Additional Context:**
   - Network connection is stable
   - Regular git clone works fine
   - Happens with all repositories

Code Review Guidelines
----------------------

For Reviewers
^^^^^^^^^^^^^

**What to Look For:**
* Correctness and logic errors
* Performance implications
* Security considerations
* Code style and consistency
* Test coverage
* Documentation completeness

**Review Checklist:**
- [ ] Code follows project style guidelines
- [ ] All functions have appropriate documentation
- [ ] Tests cover new functionality
- [ ] No obvious security issues
- [ ] Performance impact is acceptable
- [ ] Documentation is updated
- [ ] Backward compatibility is maintained

For Contributors
^^^^^^^^^^^^^^^^

**Responding to Reviews:**
* Address all feedback
* Ask questions if unclear
* Explain design decisions
* Update code and tests as needed
* Thank reviewers for their time

**Making Changes:**
* Create new commits for review changes
* Don't force-push after reviews start
* Update PR description if scope changes
* Test thoroughly after changes

Release Process
---------------

Version Management
^^^^^^^^^^^^^^^^^^

**Semantic Versioning:**
* Major: Breaking changes
* Minor: New features (backward compatible)
* Patch: Bug fixes

**Release Checklist:**
1. Update version numbers
2. Update changelog
3. Run comprehensive tests
4. Build documentation
5. Create release notes
6. Tag release
7. Update package managers

**Contributing to Releases:**
* Test release candidates
* Update documentation
* Report issues promptly
* Help with migration guides

Recognition
-----------

Contributors are recognized in:
* README.md contributors section
* Release notes
* Git commit history
* Documentation credits

Thank you for contributing to git-cache!