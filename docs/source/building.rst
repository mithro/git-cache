Building from Source
===================

This guide covers building git-cache from source code, including development setup and advanced build options.

Quick Build
-----------

For most users, building is straightforward:

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/mithro/git-cache.git
   cd git-cache
   
   # Build
   make cache
   
   # Install (optional)
   sudo make install

Build System
------------

Makefile Targets
^^^^^^^^^^^^^^^^

git-cache uses a comprehensive Makefile with multiple targets:

**Primary Targets:**

.. code-block:: bash

   make cache           # Build main git-cache binary
   make all            # Build all components
   make clean          # Remove build artifacts
   make install        # Install system-wide
   make uninstall      # Remove installed files

**Test Targets:**

.. code-block:: bash

   make github         # Build GitHub API test program
   make url-test       # Build URL parsing tests
   make fork-test      # Build fork integration tests
   make cache-test     # Run integration tests
   make test-all       # Run comprehensive test suite

**Development Targets:**

.. code-block:: bash

   make debug          # Build with debug symbols
   make clean-cache    # Clean test cache directories
   make clean-all      # Complete cleanup
   make help           # Show all available targets

Build Configuration
^^^^^^^^^^^^^^^^^^^

The build system supports several configuration options:

**Compiler Settings:**

.. code-block:: makefile

   # Default configuration
   CC = gcc
   CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2
   LDFLAGS = -lcurl -ljson-c

**Custom Compiler:**

.. code-block:: bash

   # Use Clang instead of GCC
   make CC=clang cache
   
   # Use specific GCC version
   make CC=gcc-11 cache

**Debug Build:**

.. code-block:: bash

   # Build with debug symbols and no optimization
   make CFLAGS="-Wall -Wextra -std=c99 -pedantic -g -O0" cache

**Installation Prefix:**

.. code-block:: bash

   # Install to custom location
   make install PREFIX=/opt/git-cache
   
   # Install to user directory
   make install PREFIX=$HOME/.local

Dependencies
------------

Required Dependencies
^^^^^^^^^^^^^^^^^^^^^

**Build Tools:**
* GCC 7.0+ or Clang 6.0+
* GNU Make 4.0+
* Git 2.20+

**System Libraries:**
* libcurl (with development headers)
* libjson-c (with development headers)

**Installation by Platform:**

Ubuntu/Debian:

.. code-block:: bash

   sudo apt-get install build-essential git libcurl4-openssl-dev libjson-c-dev

CentOS/RHEL:

.. code-block:: bash

   sudo yum install gcc make git libcurl-devel json-c-devel

Fedora:

.. code-block:: bash

   sudo dnf install gcc make git libcurl-devel json-c-devel

macOS:

.. code-block:: bash

   # Install Xcode command line tools
   xcode-select --install
   
   # Install dependencies via Homebrew
   brew install curl json-c

Alpine Linux:

.. code-block:: bash

   apk add build-base git curl-dev json-c-dev

Optional Dependencies
^^^^^^^^^^^^^^^^^^^^^

**Documentation Building:**
* Python 3.7+
* Sphinx
* Doxygen (for API documentation)

**Testing:**
* GitHub CLI (gh) - for advanced GitHub testing
* Docker - for containerized testing

**Development:**
* Valgrind - for memory debugging
* cppcheck - for static analysis
* clang-format - for code formatting

Development Setup
-----------------

Complete Development Environment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Clone with full history
   git clone https://github.com/mithro/git-cache.git
   cd git-cache
   
   # Install development dependencies
   sudo apt-get install build-essential git libcurl4-openssl-dev libjson-c-dev \
                        valgrind cppcheck clang-format doxygen
   
   # Install documentation dependencies
   pip install -r docs/requirements.txt
   
   # Build everything
   make all
   
   # Run tests
   make test-all

IDE Integration
^^^^^^^^^^^^^^^

**VS Code Setup:**

.. code-block:: json

   // .vscode/c_cpp_properties.json
   {
       "configurations": [
           {
               "name": "Linux",
               "includePath": [
                   "${workspaceFolder}/**",
                   "/usr/include/curl",
                   "/usr/include/json-c"
               ],
               "defines": ["_GNU_SOURCE"],
               "compilerPath": "/usr/bin/gcc",
               "cStandard": "c99",
               "intelliSenseMode": "linux-gcc-x64"
           }
       ],
       "version": 4
   }

**Build Task (VS Code):**

.. code-block:: json

   // .vscode/tasks.json
   {
       "version": "2.0.0",
       "tasks": [
           {
               "label": "build",
               "type": "shell",
               "command": "make",
               "args": ["cache"],
               "group": {
                   "kind": "build",
                   "isDefault": true
               },
               "presentation": {
                   "echo": true,
                   "reveal": "always",
                   "focus": false,
                   "panel": "shared"
               }
           }
       ]
   }

Debugging Builds
^^^^^^^^^^^^^^^^

**Debug Build:**

.. code-block:: bash

   # Build with debug symbols
   make clean
   make CFLAGS="-Wall -Wextra -std=c99 -pedantic -g -O0 -DDEBUG" cache

**Memory Debugging:**

.. code-block:: bash

   # Build debug version
   make debug
   
   # Run with Valgrind
   valgrind --leak-check=full --show-leak-kinds=all ./git-cache status

**Static Analysis:**

.. code-block:: bash

   # Run cppcheck
   cppcheck --enable=warning,style,performance,portability *.c
   
   # Run clang static analyzer
   scan-build make cache

Advanced Build Options
----------------------

Cross-Compilation
^^^^^^^^^^^^^^^^^

**ARM64 Cross-Compilation:**

.. code-block:: bash

   # Install cross-compiler
   sudo apt-get install gcc-aarch64-linux-gnu
   
   # Cross-compile
   make CC=aarch64-linux-gnu-gcc cache

**Static Linking:**

.. code-block:: bash

   # Build with static libraries
   make LDFLAGS="-static -lcurl -ljson-c -lssl -lcrypto -lz -lpthread" cache

Optimization Builds
^^^^^^^^^^^^^^^^^^^^

**Performance Optimized:**

.. code-block:: bash

   # Maximum optimization
   make CFLAGS="-Wall -Wextra -std=c99 -pedantic -O3 -march=native -DNDEBUG" cache

**Size Optimized:**

.. code-block:: bash

   # Minimize binary size
   make CFLAGS="-Wall -Wextra -std=c99 -pedantic -Os -DNDEBUG" cache
   strip git-cache

Sanitizer Builds
^^^^^^^^^^^^^^^^

**Address Sanitizer:**

.. code-block:: bash

   # Build with AddressSanitizer
   make CFLAGS="-Wall -Wextra -std=c99 -pedantic -g -O1 -fsanitize=address" \
        LDFLAGS="-fsanitize=address -lcurl -ljson-c" cache

**Thread Sanitizer:**

.. code-block:: bash

   # Build with ThreadSanitizer
   make CFLAGS="-Wall -Wextra -std=c99 -pedantic -g -O1 -fsanitize=thread" \
        LDFLAGS="-fsanitize=thread -lcurl -ljson-c" cache

Documentation Building
----------------------

API Documentation
^^^^^^^^^^^^^^^^^

Build comprehensive API documentation:

.. code-block:: bash

   # Install documentation dependencies
   pip install -r docs/requirements.txt
   
   # Install Doxygen
   sudo apt-get install doxygen  # Ubuntu/Debian
   brew install doxygen          # macOS
   
   # Build documentation
   cd docs
   make html
   
   # Serve locally
   make livehtml

**Output Location:**
* HTML: ``docs/build/html/index.html``
* API Reference: ``docs/build/html/api/library_root.html``

**Documentation Features:**
* Automatic API documentation from C source
* Interactive code examples
* Search functionality
* Mobile-responsive design

Manual Pages
^^^^^^^^^^^^

.. code-block:: bash

   # Generate man pages (future feature)
   make man
   
   # Install man pages
   sudo make install-man

Packaging
---------

Debian Package
^^^^^^^^^^^^^^

.. code-block:: bash

   # Install packaging tools
   sudo apt-get install build-essential debhelper dh-make
   
   # Create package structure
   dh_make --createorig --single --packagename git-cache_1.0.0
   
   # Build package
   dpkg-buildpackage -us -uc

RPM Package
^^^^^^^^^^^

.. code-block:: bash

   # Install packaging tools
   sudo yum install rpm-build rpmdevtools
   
   # Set up build environment
   rpmdev-setuptree
   
   # Create spec file and build
   rpmbuild -ba git-cache.spec

Docker Image
^^^^^^^^^^^^

.. code-block:: dockerfile

   FROM ubuntu:22.04
   
   # Install dependencies
   RUN apt-get update && apt-get install -y \
       build-essential git libcurl4-openssl-dev libjson-c-dev
   
   # Copy source
   COPY . /src/git-cache
   WORKDIR /src/git-cache
   
   # Build
   RUN make cache && make install
   
   # Set up runtime environment
   ENV GIT_CACHE_ROOT=/cache
   ENV GIT_CHECKOUT_ROOT=/workspace
   
   # Create directories
   RUN mkdir -p /cache /workspace
   
   WORKDIR /workspace
   ENTRYPOINT ["git-cache"]

Build Testing
-------------

Build Verification
^^^^^^^^^^^^^^^^^^

Verify builds work correctly:

.. code-block:: bash

   # Test basic functionality
   ./git-cache --version
   ./git-cache --help
   ./git-cache status
   
   # Test with simple repository
   ./git-cache clone https://github.com/octocat/Hello-World.git

Platform Testing
^^^^^^^^^^^^^^^^

Test on multiple platforms:

.. code-block:: bash

   # Test matrix
   for platform in ubuntu:20.04 ubuntu:22.04 centos:8 fedora:36; do
       docker run --rm -v $(pwd):/src -w /src $platform bash -c '
           apt-get update && apt-get install -y build-essential libcurl4-openssl-dev libjson-c-dev ||
           yum install -y gcc make libcurl-devel json-c-devel ||
           dnf install -y gcc make libcurl-devel json-c-devel
           make cache && ./git-cache --version
       '
   done

Continuous Integration
^^^^^^^^^^^^^^^^^^^^^^

**GitHub Actions Build Matrix:**

.. code-block:: yaml

   strategy:
     matrix:
       os: [ubuntu-20.04, ubuntu-22.04]
       compiler: [gcc, clang]
       build_type: [debug, release]
   
   runs-on: ${{ matrix.os }}
   
   steps:
     - name: Install dependencies
       run: sudo apt-get install -y libcurl4-openssl-dev libjson-c-dev
     
     - name: Build
       env:
         CC: ${{ matrix.compiler }}
       run: |
         if [ "${{ matrix.build_type }}" = "debug" ]; then
           make debug
         else
           make cache
         fi

Troubleshooting Builds
----------------------

Common Build Issues
^^^^^^^^^^^^^^^^^^^

**Missing Headers:**

.. code-block:: text

   error: curl/curl.h: No such file or directory

Solution:

.. code-block:: bash

   sudo apt-get install libcurl4-openssl-dev

**Linker Errors:**

.. code-block:: text

   undefined reference to `curl_easy_init'

Solution:

.. code-block:: bash

   # Ensure proper linking order
   make clean && make cache

**Version Compatibility:**

.. code-block:: text

   error: unknown type name '_Bool'

Solution:

.. code-block:: bash

   # Use newer compiler or add compatibility flags
   make CC=gcc-9 cache

Build Environment Issues
^^^^^^^^^^^^^^^^^^^^^^^^^

**Check Build Environment:**

.. code-block:: bash

   # Verify compiler
   gcc --version
   clang --version
   
   # Check dependencies
   pkg-config --modversion libcurl
   pkg-config --modversion json-c
   
   # Verify make version
   make --version

**Clean Build:**

.. code-block:: bash

   # Complete clean rebuild
   make clean-all
   make cache

Performance Considerations
--------------------------

Build Speed Optimization
^^^^^^^^^^^^^^^^^^^^^^^^^

**Parallel Builds:**

.. code-block:: bash

   # Use multiple cores
   make -j$(nproc) cache

**Incremental Builds:**

.. code-block:: bash

   # Only rebuild changed files
   make cache  # Automatically detects changes

**ccache Integration:**

.. code-block:: bash

   # Install ccache for faster rebuilds
   sudo apt-get install ccache
   
   # Use with make
   make CC="ccache gcc" cache

Binary Size Optimization
^^^^^^^^^^^^^^^^^^^^^^^^^

**Strip Symbols:**

.. code-block:: bash

   make cache
   strip git-cache
   ls -lh git-cache

**Link-Time Optimization:**

.. code-block:: bash

   make CFLAGS="-O3 -flto" LDFLAGS="-flto -lcurl -ljson-c" cache

Next Steps
----------

* :doc:`contributing` - Contribute to the build system
* :doc:`architecture` - Understand the codebase structure
* :doc:`api/library_root` - Explore the API documentation