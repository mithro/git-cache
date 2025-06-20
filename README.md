# git-cache

[![CI](https://github.com/mithro/git-cache/actions/workflows/ci.yml/badge.svg)](https://github.com/mithro/git-cache/actions/workflows/ci.yml)
[![Test Suite](https://github.com/mithro/git-cache/actions/workflows/test.yml/badge.svg)](https://github.com/mithro/git-cache/actions/workflows/test.yml)

A high-performance Git repository caching tool that speeds up clone operations and manages shared repository storage.

## Description

git-cache is a sophisticated caching system for Git repositories that creates efficient local caches and provides instant access to repositories through reference-based clones. It significantly reduces network usage and clone times by maintaining local bare repositories and creating lightweight checkouts that share objects.

## Features

- **Smart Caching**: Maintains bare repositories as efficient local caches
- **Reference-based Clones**: Creates fast, space-efficient checkouts using Git alternates
- **Multiple Clone Strategies**: Full, shallow, treeless, and blobless clones
- **GitHub Integration**: Automatic fork management and GitHub API integration  
- **Comprehensive URL Support**: Handles all common Git URL formats (https, ssh, git+ssh, etc.)
- **Dual Checkout System**: Read-only and modifiable repository copies
- **Cache Management**: List, clean, and sync cached repositories
- **Robust Error Handling**: Graceful handling of network issues and cache corruption

## Building

Build the main git-cache tool:
```bash
make cache
```

Build all components including tests:
```bash
make all
```

## Installation

Install to system directories:
```bash
sudo make install
```

This installs `git-cache` to `/usr/local/bin/` by default.

## Usage

### Basic Operations

Clone a repository with caching:
```bash
git-cache clone https://github.com/user/repo.git
```

Show cache status and configuration:
```bash
git-cache status
```

List all cached repositories:
```bash
git-cache list
```

Clean up cache and checkouts:
```bash
git-cache clean
```

### Advanced Usage

Clone with specific strategy:
```bash
git-cache clone --strategy shallow --depth 1 https://github.com/user/repo.git
git-cache clone --strategy treeless https://github.com/user/repo.git
git-cache clone --strategy blobless https://github.com/user/repo.git
```

Verbose output:
```bash
git-cache clone --verbose https://github.com/user/repo.git
```

## Repository Structure

git-cache creates a three-tier structure:

1. **Cache** (`~/.cache/git/github.com/user/repo`): Bare repository for efficient storage
2. **Read-only Checkout** (`./github/user/repo`): Reference-based clone for browsing
3. **Modifiable Checkout** (`./github/mithro/user-repo`): Development copy with blobless strategy

## Configuration

git-cache uses environment variables for configuration:

- `GIT_CACHE`: Override cache directory (default: `~/.cache/git`)
- `GIT_CHECKOUT_ROOT`: Override checkout directory (default: `./github`)
- `GITHUB_TOKEN`: GitHub API token for private repositories and forking

## Testing

Run comprehensive test suite:
```bash
make test-all
```

Run specific test categories:
```bash
make cache-test      # Integration tests
make url-test-run    # URL parsing tests  
make github-test     # GitHub API tests
```

## Development

Build with debug symbols:
```bash
make debug
```

Run static analysis:
```bash
cppcheck --enable=warning *.c
```

## Architecture

- **git-cache.c**: Main application logic and cache management
- **github_api.c**: GitHub API integration and URL parsing
- **Comprehensive Test Suite**: 45+ tests covering all functionality
- **CI/CD Pipeline**: GitHub Actions for continuous integration

## Contributing

1. Follow Git project coding style (tabs for indentation)
2. Add tests for new functionality  
3. Ensure all tests pass with `make test-all`
4. Update documentation for user-facing changes

## License

This project is licensed under the same terms as the Git project itself.