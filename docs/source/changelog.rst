Changelog
=========

All notable changes to git-cache are documented in this file.

The format is based on `Keep a Changelog <https://keepachangelog.com/en/1.0.0/>`_,
and this project adheres to `Semantic Versioning <https://semver.org/spec/v2.0.0.html>`_.

[Unreleased]
------------

**Added**
- Comprehensive documentation with Sphinx and ReadTheDocs integration
- Automatic API documentation generation with Exhale and Doxygen
- Safe GitHub integration testing with multiple security layers
- Fork integration unit tests for testing without side effects
- Multi-layered testing strategy (unit/integration/GitHub/fork tests)

**Changed**
- Enhanced testing infrastructure with better safety boundaries
- Improved documentation organization and structure

**Fixed**
- Critical GitHub fork integration bug: modifiable checkouts now use correct fork URLs
- Progress indicators now properly clear after completion

[1.0.0] - 2025-01-15
--------------------

**Added**
- Initial release of git-cache
- Three-tier repository caching system (cache/read-only/modifiable)
- Multiple clone strategies (full, shallow, treeless, blobless)
- GitHub API integration with automatic fork creation
- Concurrent operation support with file-based locking
- Comprehensive error recovery and validation
- Progress indicators for long-running operations
- Cache synchronization across all repositories
- Enhanced list command with detailed repository information
- Initial submodule support with --recursive flag
- Robust testing infrastructure
- CI/CD pipeline with GitHub Actions

**Core Features**
- Smart caching with bare repository storage
- Reference-based checkouts using Git alternates
- GitHub fork management with organization support
- Privacy settings for forked repositories
- Network retry with exponential backoff
- Disk space validation before operations
- Deep git repository integrity validation
- URL parsing for all common Git formats

**GitHub Integration**
- Personal Access Token authentication
- Automatic fork creation for development workflows
- Organization fork management
- Repository privacy control
- API error handling and rate limiting awareness
- Fork URL resolution and fallback logic

**Caching System**
- Efficient object sharing via Git alternates
- Space-optimized storage (3x+ reduction typical)
- Atomic operations with backup/restore
- Cache corruption detection and recovery
- Stale lock detection with PID tracking
- Safe concurrent operations

**Testing Infrastructure**
- Unit tests for core functionality
- Integration tests with real Git operations
- GitHub API testing with authentication
- Fork operation testing (with safety controls)
- Concurrent operation testing
- URL parsing validation
- Error condition testing

Previous Versions
-----------------

Development History
^^^^^^^^^^^^^^^^^^^

The development of git-cache followed a structured implementation plan:

**Phase 1: Core Infrastructure** (Completed)
- Basic git subcommand framework with argument parsing
- Cache directory management with automatic creation
- Comprehensive GitHub URL parsing (HTTPS/SSH)
- Concurrent execution support with file-based locking

**Phase 2: Caching Implementation** (98% Complete)
- Full bare repository caching
- Reference-based checkouts with all strategies
- Object sharing and storage optimization
- Comprehensive error recovery and validation
- Progress indicators for user feedback

**Phase 3: GitHub Integration** (90% Complete)
- Complete GitHub API client with authentication
- Fork creation and management
- Privacy settings control
- Fork URL integration for modifiable checkouts

**Phase 4: Git Submodule Support** (5% Complete)
- Initial --recursive flag implementation
- Core submodule implementation pending

**Phase 5: Advanced Features** (50% Complete)
- Cache sync functionality implemented
- Enhanced list command with detailed information
- Remaining: auto-strategy detection, configuration files

Migration Guide
---------------

From Development Versions
^^^^^^^^^^^^^^^^^^^^^^^^^^

If you were using development versions of git-cache, please note:

**Breaking Changes in 1.0.0:**
- None - First stable release

**Configuration Changes:**
- Environment variable support added
- No configuration file format changes (none existed yet)

**API Changes:**
- Public API formalized
- Internal functions marked as static
- Struct definitions stabilized

**Testing Changes:**
- Test infrastructure completely rewritten
- New safety boundaries for GitHub testing
- Comprehensive test coverage added

Upgrade Instructions
--------------------

From Source Builds
^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Backup existing installation
   which git-cache && cp $(which git-cache) git-cache.backup
   
   # Clean rebuild
   make clean
   git pull
   make cache
   
   # Update installation
   sudo make install
   
   # Verify upgrade
   git-cache --version

Dependencies
^^^^^^^^^^^^

**New Dependencies in 1.0.0:**
- No new required dependencies
- Optional: Doxygen for documentation building
- Optional: Sphinx for documentation building

**Deprecated Dependencies:**
- None in this release

Known Issues
------------

Current Limitations
^^^^^^^^^^^^^^^^^^^

**Submodule Support:**
- Recursive submodule caching not yet implemented
- Only basic --recursive flag support available
- Full submodule reference chains planned for 1.1.0

**Configuration:**
- Configuration file support planned for 1.1.0
- Currently uses environment variables only

**Platform Support:**
- Primary support: Linux
- Experimental: macOS, Windows WSL
- Native Windows support planned

**GitHub Integration:**
- GitLab and Bitbucket support planned
- Webhook integration planned for 1.2.0

Compatibility
-------------

**Git Versions:**
- Required: Git 2.20+
- Recommended: Git 2.30+
- Tested with: Git 2.34, 2.39, 2.42

**Operating Systems:**
- Ubuntu 20.04, 22.04 (primary support)
- Debian 11, 12
- CentOS 8, 9
- Fedora 36, 37
- macOS 12+ (experimental)
- Windows WSL2 (experimental)

**Compilers:**
- GCC 7.0+ (primary)
- Clang 6.0+ (tested)
- MSVC (planned)

Roadmap
-------

Planned Features
^^^^^^^^^^^^^^^^

**Version 1.1.0** (Next Minor Release)
- Complete submodule support with recursive caching
- Configuration file system (.gitcacherc)
- Auto-detection of optimal clone strategies
- Cache repair mechanisms
- Performance optimizations

**Version 1.2.0**
- GitLab and Bitbucket integration
- Webhook support for real-time updates
- Plugin architecture foundation
- Distributed cache sharing
- Advanced monitoring and metrics

**Version 2.0.0** (Future Major Release)
- Plugin system with third-party provider support
- Configuration migration tools
- Advanced caching algorithms
- Web-based management interface
- Enterprise features

Security Updates
----------------

**Security Policy:**
- Security issues are addressed in patch releases
- Critical security fixes may result in emergency releases
- Security advisories published for all security-related updates

**Reporting Security Issues:**
- Email: security@git-cache-project.org
- GPG key available for encrypted communication
- Responsible disclosure policy in effect

Contributing to Changelog
--------------------------

**Guidelines for Contributors:**
- Add entries to [Unreleased] section
- Use Keep a Changelog format
- Include issue/PR references where relevant
- Categorize changes appropriately (Added/Changed/Deprecated/Removed/Fixed/Security)

**Example Entry:**

.. code-block:: text

   **Added**
   - New feature description (#123)
   - Another feature with details
   
   **Fixed**
   - Bug fix description (#124)
   - Security fix (CVE-2025-XXXX)

**Changelog Maintenance:**
- Entries moved from [Unreleased] to versioned sections on release
- Breaking changes clearly marked
- Migration instructions provided for significant changes
- Dependencies and compatibility information updated

For detailed development history, see the `Git commit log <https://github.com/mithro/git-cache/commits/main>`_ and `GitHub releases <https://github.com/mithro/git-cache/releases>`_.