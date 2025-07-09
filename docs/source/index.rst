git-cache Documentation
======================

.. image:: https://github.com/mithro/git-cache/actions/workflows/ci.yml/badge.svg
   :target: https://github.com/mithro/git-cache/actions/workflows/ci.yml
   :alt: CI Status

.. image:: https://github.com/mithro/git-cache/actions/workflows/test.yml/badge.svg
   :target: https://github.com/mithro/git-cache/actions/workflows/test.yml
   :alt: Test Suite

.. image:: https://readthedocs.org/projects/git-cache/badge/?version=latest
   :target: https://git-cache.readthedocs.io/en/latest/?badge=latest
   :alt: Documentation Status

.. warning::
   **Experimental Project**
   
   This project is an experiment in vibe coding a new greenfield project using Claude Code. Despite the impression that this code might work, expect it to be horribly broken in strange and unusual ways. Don't trust it with your code repositories!

A high-performance Git repository caching tool that speeds up clone operations and manages shared repository storage.

Overview
--------

git-cache is a sophisticated caching system for Git repositories that creates efficient local caches and provides instant access to repositories through reference-based clones. It significantly reduces network usage and clone times by maintaining local bare repositories and creating lightweight checkouts that share objects.

Key Features
-----------

* **Smart Caching**: Maintains bare repositories as efficient local caches
* **Reference-based Clones**: Creates fast, space-efficient checkouts using Git alternates
* **Multiple Clone Strategies**: Full, shallow, treeless, and blobless clones
* **GitHub Integration**: Automatic fork management and GitHub API integration  
* **Comprehensive URL Support**: Handles all common Git URL formats (https, ssh, git+ssh, etc.)
* **Dual Checkout System**: Read-only and modifiable repository copies
* **Cache Management**: List, clean, and sync cached repositories
* **Robust Error Handling**: Graceful handling of network issues and cache corruption
* **Concurrent Operations**: File-based locking for safe parallel execution
* **Progress Indicators**: Visual feedback for long-running operations

Quick Start
-----------

Installation
^^^^^^^^^^^^

.. code-block:: bash

   # Build from source
   git clone https://github.com/mithro/git-cache.git
   cd git-cache
   make cache
   sudo make install

Basic Usage
^^^^^^^^^^^

.. code-block:: bash

   # Clone a repository with caching
   git-cache clone https://github.com/user/repo.git

   # Show cache status
   git-cache status

   # List cached repositories
   git-cache list

   # Synchronize all cached repositories
   git-cache sync

   # Clean up cache
   git-cache clean

Architecture
------------

Repository Structure
^^^^^^^^^^^^^^^^^^^

git-cache manages repositories in three distinct locations:

1. **Cache Repository**: ``.cache/git/github.com/<username>/<repo>``
   
   * Full bare repository containing ALL git objects
   * Acts as complete local object store and reference mirror
   * Shared across multiple checkouts for storage efficiency

2. **Read-Only Checkout**: ``github/<username>/<repo>``
   
   * Shallow/treeless/blobless working directory checkout
   * References cache repository using ``--reference`` mechanism
   * Optimized for browsing and inspection

3. **Modifiable Checkout**: ``github/mithro/<username>-<repo>``
   
   * Partial clone working directory for development
   * Connected to forked repository for push operations
   * Supports development workflows with shared object storage

Table of Contents
-----------------

.. toctree::
   :maxdepth: 2
   :caption: User Guide

   installation
   usage
   configuration
   github_integration
   testing

.. toctree::
   :maxdepth: 2
   :caption: Developer Guide

   building
   architecture
   contributing
   api/library_root

.. toctree::
   :maxdepth: 1
   :caption: Reference

   changelog
   license

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`