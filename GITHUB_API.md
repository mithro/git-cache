# GitHub API Integration

This module provides a C library for interacting with the GitHub API, specifically designed for repository forking and management operations required by the git cache tool.

## Features

- **Repository Operations**: Get repository information, fork repositories, manage privacy settings
- **Authentication**: Token-based authentication with validation
- **URL Parsing**: Support for multiple GitHub URL formats (HTTPS, SSH, bare)
- **Error Handling**: Comprehensive error codes and messages
- **Memory Management**: Clean allocation and deallocation of all structures

## API Reference

### Core Structures

```c
struct github_client;      // API client with authentication
struct github_repo;        // Repository information
struct github_response;    // HTTP response data
```

### Client Management

```c
// Create and destroy API client
struct github_client* github_client_create(const char *token);
void github_client_destroy(struct github_client *client);

// Configure client
int github_client_set_timeout(struct github_client *client, int timeout_seconds);
int github_validate_token(struct github_client *client);
```

### Repository Operations

```c
// Get repository information
int github_get_repo(struct github_client *client, const char *owner, const char *repo, 
                   struct github_repo **result);

// Fork repository (optionally to organization)
int github_fork_repo(struct github_client *client, const char *owner, const char *repo, 
                     const char *organization, struct github_repo **result);

// Set repository privacy
int github_set_repo_private(struct github_client *client, const char *owner, const char *repo, 
                           int is_private);
```

### Utility Functions

```c
// Parse GitHub URLs to extract owner/repo
int github_parse_repo_url(const char *url, char **owner, char **repo);

// Get human-readable error message
const char* github_get_error_string(int error_code);
```

## Error Codes

- `GITHUB_SUCCESS` (0): Operation completed successfully
- `GITHUB_ERROR_MEMORY` (-1): Memory allocation failed
- `GITHUB_ERROR_NETWORK` (-2): Network/HTTP error
- `GITHUB_ERROR_AUTH` (-3): Authentication failed
- `GITHUB_ERROR_NOT_FOUND` (-4): Repository not found
- `GITHUB_ERROR_FORBIDDEN` (-5): Access forbidden
- `GITHUB_ERROR_JSON` (-6): JSON parsing error
- `GITHUB_ERROR_INVALID` (-7): Invalid parameters

## Usage Examples

### Basic Repository Information

```c
#include "github_api.h"

int main() {
    // Create client
    struct github_client *client = github_client_create("ghp_your_token_here");
    if (!client) {
        printf("Failed to create client\n");
        return 1;
    }
    
    // Get repository info
    struct github_repo *repo;
    int ret = github_get_repo(client, "octocat", "Hello-World", &repo);
    
    if (ret == GITHUB_SUCCESS) {
        printf("Repository: %s\n", repo->full_name);
        printf("Clone URL: %s\n", repo->clone_url);
        printf("Is Private: %s\n", repo->is_private ? "Yes" : "No");
        
        github_repo_destroy(repo);
    } else {
        printf("Error: %s\n", github_get_error_string(ret));
    }
    
    github_client_destroy(client);
    return 0;
}
```

### Repository Forking

```c
#include "github_api.h"

int main() {
    struct github_client *client = github_client_create("ghp_your_token_here");
    
    // Fork to personal account
    struct github_repo *fork;
    int ret = github_fork_repo(client, "upstream-owner", "repo-name", NULL, &fork);
    
    if (ret == GITHUB_SUCCESS) {
        printf("Forked to: %s\n", fork->full_name);
        
        // Make the fork private
        ret = github_set_repo_private(client, fork->owner, fork->name, 1);
        if (ret == GITHUB_SUCCESS) {
            printf("Fork set to private\n");
        }
        
        github_repo_destroy(fork);
    }
    
    // Fork to organization
    ret = github_fork_repo(client, "upstream-owner", "repo-name", "mithro-mirrors", &fork);
    if (ret == GITHUB_SUCCESS) {
        printf("Forked to organization: %s\n", fork->full_name);
        github_repo_destroy(fork);
    }
    
    github_client_destroy(client);
    return 0;
}
```

### URL Parsing

```c
#include "github_api.h"

int main() {
    char *owner, *repo;
    const char *urls[] = {
        "https://github.com/user/repo.git",
        "git@github.com:user/repo.git",
        "github.com/user/repo",
        NULL
    };
    
    for (int i = 0; urls[i]; i++) {
        int ret = github_parse_repo_url(urls[i], &owner, &repo);
        if (ret == GITHUB_SUCCESS) {
            printf("URL: %s -> %s/%s\n", urls[i], owner, repo);
            free(owner);
            free(repo);
        } else {
            printf("Failed to parse: %s\n", urls[i]);
        }
    }
    
    return 0;
}
```

## Authentication

The GitHub API requires a personal access token for authentication. You can create one at:
https://github.com/settings/tokens

### Required Permissions

For the git cache tool, the token needs these scopes:

- `repo`: Full repository access (for forking and privacy settings)
- `read:org`: Read organization membership (for organization forks)

### Token Security

- Store tokens securely (environment variables, config files with restricted permissions)
- Never commit tokens to version control
- Use minimal required permissions
- Rotate tokens regularly

## Building and Testing

### Dependencies

- libcurl: HTTP client library
- json-c: JSON parsing library

### Building

```bash
make github        # Build GitHub test program
make github-test   # Run GitHub API tests
```

### Testing

```bash
# Basic tests (no authentication required)
./github_test

# Token validation tests
./github_test ghp_your_token_here

# Full tests including forking
./github_test ghp_your_token_here owner repo-name

# Automated test suite
export GITHUB_TOKEN=ghp_your_token_here
export TEST_REPO_OWNER=octocat
export TEST_REPO_NAME=Hello-World
./tests/run_github_tests.sh
```

## Integration with Git Cache Tool

This GitHub API client is designed to integrate with the git cache tool as described in the implementation plan:

1. **Repository Forking**: Automatically fork repositories to the mithro-mirrors organization
2. **Privacy Control**: Set forked repositories to private
3. **URL Processing**: Parse various GitHub URL formats for cache key generation
4. **Error Handling**: Provide clear feedback for network and authentication issues

### Integration Points

```c
// Example integration in git cache tool
int cache_setup_github_fork(const char *original_url) {
    char *owner, *repo;
    int ret = github_parse_repo_url(original_url, &owner, &repo);
    if (ret != GITHUB_SUCCESS) {
        return ret;
    }
    
    struct github_client *client = github_client_create(get_github_token());
    if (!client) {
        free(owner);
        free(repo);
        return GITHUB_ERROR_MEMORY;
    }
    
    // Fork to organization
    struct github_repo *fork;
    ret = github_fork_repo(client, owner, repo, "mithro-mirrors", &fork);
    
    if (ret == GITHUB_SUCCESS) {
        // Set to private
        github_set_repo_private(client, fork->owner, fork->name, 1);
        
        // Store fork URL for later use
        store_fork_url(fork->clone_url);
        
        github_repo_destroy(fork);
    }
    
    github_client_destroy(client);
    free(owner);
    free(repo);
    return ret;
}
```

## Thread Safety

The current implementation is **not thread-safe**. Each thread should use its own `github_client` instance. Future versions may add thread-safety if needed.

## Rate Limiting

The GitHub API has rate limits. The client includes:

- Configurable timeout settings
- Proper HTTP status code handling
- Error messages for rate limit responses

For production use, consider implementing:
- Exponential backoff on rate limit errors
- Request caching
- Multiple token rotation

## Memory Management

All functions that allocate memory require explicit cleanup:

```c
// Always pair create/destroy calls
struct github_client *client = github_client_create(token);
// ... use client ...
github_client_destroy(client);

// Free parsed URL components
char *owner, *repo;
github_parse_repo_url(url, &owner, &repo);
// ... use owner/repo ...
free(owner);
free(repo);

// Free repository structures
struct github_repo *repo;
github_get_repo(client, owner, name, &repo);
// ... use repo ...
github_repo_destroy(repo);
```

## Future Enhancements

Potential improvements for future versions:

- Async/non-blocking operations
- Batch operations
- Webhook support
- Extended repository metadata
- Support for other git hosting services
- Caching layer for API responses