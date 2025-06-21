Installation
============

This page covers how to install git-cache on various systems.

Prerequisites
-------------

System Requirements
^^^^^^^^^^^^^^^^^^^

* **Operating System**: Linux (primary), macOS (experimental), Windows WSL (experimental)
* **Compiler**: GCC 7.0+ or Clang 6.0+
* **Git**: Git 2.20+ (for proper alternates and partial clone support)
* **Make**: GNU Make 4.0+

Required Libraries
^^^^^^^^^^^^^^^^^^

git-cache depends on the following system libraries:

* **libcurl**: For HTTP/HTTPS GitHub API operations
* **libjson-c**: For JSON parsing of GitHub API responses

Ubuntu/Debian
^^^^^^^^^^^^^

.. code-block:: bash

   sudo apt-get update
   sudo apt-get install build-essential git libcurl4-openssl-dev libjson-c-dev

CentOS/RHEL/Fedora
^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # CentOS/RHEL
   sudo yum install gcc make git libcurl-devel json-c-devel

   # Fedora
   sudo dnf install gcc make git libcurl-devel json-c-devel

macOS
^^^^^

.. code-block:: bash

   # Install Xcode command line tools
   xcode-select --install

   # Install dependencies via Homebrew
   brew install curl json-c

   # Or via MacPorts
   sudo port install curl json-c

Building from Source
--------------------

Clone Repository
^^^^^^^^^^^^^^^^

.. code-block:: bash

   git clone https://github.com/mithro/git-cache.git
   cd git-cache

Build Options
^^^^^^^^^^^^^

Basic Build
"""""""""""

.. code-block:: bash

   # Build the main git-cache binary
   make cache

   # Build all components (including tests)
   make all

Debug Build
"""""""""""

.. code-block:: bash

   # Build with debug symbols and optimizations disabled
   make debug

Install System-wide
^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Install to /usr/local/bin (requires sudo)
   sudo make install

   # Install to custom prefix
   make install PREFIX=/opt/git-cache

   # Install to user directory (no sudo required)
   make install PREFIX=$HOME/.local

Verify Installation
^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Check version
   git-cache --version

   # Show help
   git-cache --help

   # Test basic functionality
   git-cache status

Development Setup
-----------------

For development work, you may want to build additional components:

Testing Infrastructure
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Build all test programs
   make github      # GitHub API tests
   make url-test    # URL parsing tests
   make fork-test   # Fork integration tests

   # Run test suites
   make cache-test        # Integration tests
   make fork-test-run     # Fork logic unit tests
   make url-test-run      # URL parsing tests

Documentation
^^^^^^^^^^^^^

.. code-block:: bash

   # Install documentation dependencies
   pip install -r docs/requirements.txt

   # Build documentation
   cd docs
   make html

   # Serve documentation locally
   make livehtml

Configuration
-------------

Environment Variables
^^^^^^^^^^^^^^^^^^^^^

git-cache can be configured through environment variables:

.. code-block:: bash

   # Override cache directory (default: ~/.cache/git)
   export GIT_CACHE_ROOT=/path/to/cache

   # Override checkout directory (default: ./github)
   export GIT_CHECKOUT_ROOT=/path/to/checkouts

   # GitHub API token for private repositories and forking
   export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

Shell Integration
^^^^^^^^^^^^^^^^^

Add git-cache to your shell's PATH:

.. code-block:: bash

   # Add to ~/.bashrc or ~/.zshrc
   export PATH="/usr/local/bin:$PATH"

   # Or if installed to user directory
   export PATH="$HOME/.local/bin:$PATH"

Troubleshooting
---------------

Common Issues
^^^^^^^^^^^^^

Missing Libraries
"""""""""""""""""

If you get compilation errors about missing headers:

.. code-block:: bash

   # Error: curl/curl.h: No such file or directory
   sudo apt-get install libcurl4-openssl-dev

   # Error: json-c/json.h: No such file or directory
   sudo apt-get install libjson-c-dev

Permission Errors
"""""""""""""""""

If you get permission errors during installation:

.. code-block:: bash

   # Install to user directory instead
   make install PREFIX=$HOME/.local

   # Or fix permissions (not recommended)
   sudo chown -R $USER:$USER /usr/local

Build Failures
""""""""""""""

If the build fails:

.. code-block:: bash

   # Clean and rebuild
   make clean
   make cache

   # Check compiler version
   gcc --version

   # Verbose build for debugging
   make cache V=1

Testing Installation
^^^^^^^^^^^^^^^^^^^^

Verify git-cache works correctly:

.. code-block:: bash

   # Test basic commands
   git-cache --version
   git-cache status
   git-cache list

   # Test with a small repository
   git-cache clone https://github.com/octocat/Hello-World.git

Uninstallation
--------------

To remove git-cache:

.. code-block:: bash

   # Remove binary
   sudo make uninstall

   # Remove cache directories (optional)
   rm -rf ~/.cache/git
   rm -rf ./github

   # Remove configuration (if using custom config)
   rm -f ~/.gitcacherc

Next Steps
----------

After installation, see:

* :doc:`usage` - Learn how to use git-cache
* :doc:`configuration` - Configure git-cache for your workflow
* :doc:`github_integration` - Set up GitHub integration