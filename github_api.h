#ifndef GITHUB_API_H
#define GITHUB_API_H

/**
 * @file github_api.h
 * @brief GitHub API integration for git-cache
 * 
 * This module provides GitHub API functionality for git-cache, including
 * repository operations, fork management, and authentication.
 */

#include <stddef.h>

/** @brief Base URL for GitHub API requests */
#define GITHUB_API_BASE_URL "https://api.github.com"
/** @brief Maximum length for GitHub URLs */
#define GITHUB_MAX_URL_LEN 512
/** @brief Maximum length for GitHub tokens */
#define GITHUB_MAX_TOKEN_LEN 256
/** @brief Maximum length for GitHub API responses */
#define GITHUB_MAX_RESPONSE_LEN 65536

/**
 * @brief GitHub API response structure
 * 
 * Contains the response data, size, status code, and any error messages
 * from GitHub API requests.
 */
struct github_response {
	char *data;           /**< Response data from GitHub API */
	size_t size;          /**< Size of response data */
	long status_code;     /**< HTTP status code */
	char *error_message;  /**< Error message if request failed */
};

/**
 * @brief GitHub repository information structure
 * 
 * Contains metadata about a GitHub repository including ownership,
 * URLs, and repository properties.
 */
struct github_repo {
	char *owner;      /**< Repository owner username */
	char *name;       /**< Repository name */
	char *full_name;  /**< Full repository name (owner/name) */
	char *clone_url;  /**< HTTPS clone URL */
	char *ssh_url;    /**< SSH clone URL */
	int is_fork;      /**< Whether repository is a fork */
	int is_private;   /**< Whether repository is private */
	int fork_count;   /**< Number of forks */
};

/**
 * @brief GitHub API client structure
 * 
 * Manages authentication and configuration for GitHub API requests.
 */
struct github_client {
	char *token;      /**< GitHub personal access token */
	char *user_agent; /**< User agent string for requests */
	int timeout;      /**< Request timeout in seconds */
};

/* Function prototypes */

/**
 * @defgroup client_management Client Management
 * @brief Functions for creating and managing GitHub API clients
 * @{
 */

/**
 * @brief Create a new GitHub API client
 * @param token GitHub personal access token
 * @return Pointer to new client, or NULL on failure
 */
struct github_client* github_client_create(const char *token);

/**
 * @brief Destroy a GitHub API client and free resources
 * @param client Client to destroy
 */
void github_client_destroy(struct github_client *client);

/**
 * @brief Set timeout for GitHub API requests
 * @param client GitHub API client
 * @param timeout_seconds Timeout in seconds
 * @return 0 on success, negative on error
 */
int github_client_set_timeout(struct github_client *client, int timeout_seconds);

/** @} */

/**
 * @defgroup repository_operations Repository Operations
 * @brief Functions for GitHub repository management
 * @{
 */

/**
 * @brief Get repository information from GitHub
 * @param client GitHub API client
 * @param owner Repository owner username
 * @param repo Repository name
 * @param result Pointer to store repository information
 * @return 0 on success, negative error code on failure
 */
int github_get_repo(struct github_client *client, const char *owner, const char *repo, 
	               struct github_repo **result);

/**
 * @brief Fork a repository on GitHub
 * @param client GitHub API client
 * @param owner Original repository owner
 * @param repo Repository name to fork
 * @param organization Optional organization to fork to (NULL for user)
 * @param result Pointer to store forked repository information
 * @return 0 on success, negative error code on failure
 */
int github_fork_repo(struct github_client *client, const char *owner, const char *repo, 
	                 const char *organization, struct github_repo **result);

/**
 * @brief Set repository privacy status
 * @param client GitHub API client
 * @param owner Repository owner username
 * @param repo Repository name
 * @param is_private 1 for private, 0 for public
 * @return 0 on success, negative error code on failure
 */
int github_set_repo_private(struct github_client *client, const char *owner, const char *repo, 
	                       int is_private);

/** @} */

/**
 * @defgroup utility_functions Utility Functions
 * @brief Helper functions for GitHub operations
 * @{
 */

/**
 * @brief Destroy repository structure and free resources
 * @param repo Repository structure to destroy
 */
void github_repo_destroy(struct github_repo *repo);

/**
 * @brief Parse GitHub repository URL to extract owner and name
 * @param url GitHub repository URL
 * @param owner Pointer to store owner name (caller must free)
 * @param repo Pointer to store repository name (caller must free)
 * @return 0 on success, negative on error
 */
int github_parse_repo_url(const char *url, char **owner, char **repo);

/**
 * @brief Validate GitHub API token
 * @param client GitHub API client with token
 * @return 0 if valid, negative error code if invalid
 */
int github_validate_token(struct github_client *client);

/**
 * @brief Get human-readable error message for error code
 * @param error_code Error code from GitHub API functions
 * @return Error message string
 */
const char* github_get_error_string(int error_code);

/** @} */

/* Error codes */
#define GITHUB_SUCCESS          0
#define GITHUB_ERROR_MEMORY     -1
#define GITHUB_ERROR_NETWORK    -2
#define GITHUB_ERROR_AUTH       -3
#define GITHUB_ERROR_NOT_FOUND  -4
#define GITHUB_ERROR_FORBIDDEN  -5
#define GITHUB_ERROR_JSON       -6
#define GITHUB_ERROR_INVALID    -7

#endif /* GITHUB_API_H */