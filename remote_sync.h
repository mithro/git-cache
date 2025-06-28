#ifndef REMOTE_SYNC_H
#define REMOTE_SYNC_H

/**
 * @file remote_sync.h
 * @brief Remote mirror management and synchronization for git-cache
 * 
 * This module provides functionality for managing multiple remote mirrors
 * and synchronizing repositories across different remote locations for
 * redundancy and performance optimization.
 */

#include "git-cache.h"
#include <time.h>

/* Forward declarations */
struct repo_info;
struct cache_config;

/**
 * @brief Remote mirror information
 */
struct remote_mirror {
	char *name;           /**< Mirror name/identifier */
	char *url;            /**< Mirror URL */
	char *type;           /**< Mirror type (backup, performance, etc.) */
	int priority;         /**< Mirror priority (0=highest) */
	int enabled;          /**< Whether mirror is enabled */
	time_t last_sync;     /**< Last synchronization time */
	int sync_status;      /**< Last sync status (0=success) */
	char *sync_error;     /**< Last sync error message */
	struct remote_mirror *next; /**< Next mirror in list */
};

/**
 * @brief Remote synchronization configuration
 */
struct sync_config {
	int auto_sync;              /**< Enable automatic synchronization */
	int sync_interval_hours;    /**< Sync interval in hours */
	int max_concurrent_syncs;   /**< Maximum concurrent sync operations */
	int retry_count;            /**< Number of retry attempts */
	int retry_delay_seconds;    /**< Delay between retries */
	char *preferred_mirror;     /**< Preferred mirror for new clones */
	int fallback_enabled;       /**< Enable fallback to other mirrors */
};

/**
 * @brief Sync operation result
 */
struct sync_result {
	int success_count;    /**< Number of successful syncs */
	int error_count;      /**< Number of failed syncs */
	int skipped_count;    /**< Number of skipped syncs */
	time_t start_time;    /**< Sync operation start time */
	time_t end_time;      /**< Sync operation end time */
	char *error_summary;  /**< Summary of errors encountered */
};

/**
 * @brief Remote sync error codes
 */
#define SYNC_SUCCESS           0
#define SYNC_ERROR_NETWORK    -1
#define SYNC_ERROR_AUTH       -2
#define SYNC_ERROR_NOT_FOUND  -3
#define SYNC_ERROR_CONFLICT   -4
#define SYNC_ERROR_TIMEOUT    -5
#define SYNC_ERROR_INVALID    -6
#define SYNC_ERROR_MEMORY     -7

/**
 * @brief Add remote mirror to repository
 * @param repo Repository information
 * @param mirror_name Name for the mirror
 * @param mirror_url URL of the mirror
 * @param mirror_type Type of mirror (backup, performance, etc.)
 * @param priority Mirror priority (0=highest)
 * @return SYNC_SUCCESS on success, error code on failure
 */
int add_remote_mirror(struct repo_info *repo, const char *mirror_name,
                     const char *mirror_url, const char *mirror_type, int priority);

/**
 * @brief Remove remote mirror from repository
 * @param repo Repository information
 * @param mirror_name Name of mirror to remove
 * @return SYNC_SUCCESS on success, error code on failure
 */
int remove_remote_mirror(struct repo_info *repo, const char *mirror_name);

/**
 * @brief List all remote mirrors for repository
 * @param repo Repository information
 * @param mirrors Output pointer to mirror list
 * @return Number of mirrors found, or negative error code
 */
int list_remote_mirrors(const struct repo_info *repo, struct remote_mirror **mirrors);

/**
 * @brief Synchronize repository with all configured mirrors
 * @param repo Repository information
 * @param config Synchronization configuration
 * @param result Output structure for sync results
 * @return SYNC_SUCCESS on success, error code on failure
 */
int sync_with_mirrors(struct repo_info *repo, const struct sync_config *config,
                     struct sync_result *result);

/**
 * @brief Synchronize repository with specific mirror
 * @param repo Repository information
 * @param mirror_name Name of mirror to sync with
 * @param force Force sync even if up to date
 * @return SYNC_SUCCESS on success, error code on failure
 */
int sync_with_mirror(struct repo_info *repo, const char *mirror_name, int force);

/**
 * @brief Auto-discover available mirrors for repository
 * @param repo Repository information
 * @param discovered_mirrors Output pointer to discovered mirrors
 * @return Number of mirrors found, or negative error code
 */
int discover_mirrors(const struct repo_info *repo, struct remote_mirror **discovered_mirrors);

/**
 * @brief Test connectivity to all configured mirrors
 * @param repo Repository information
 * @param results Output array of test results
 * @return Number of mirrors tested, or negative error code
 */
int test_mirror_connectivity(const struct repo_info *repo, int **results);

/**
 * @brief Get optimal mirror for operation based on performance
 * @param repo Repository information
 * @param operation_type Type of operation (clone, fetch, push)
 * @return Best mirror name, or NULL if none available
 */
const char* get_optimal_mirror(const struct repo_info *repo, const char *operation_type);

/**
 * @brief Push repository changes to all mirrors
 * @param repo Repository information
 * @param branch Branch to push (NULL for all branches)
 * @param force Force push
 * @return SYNC_SUCCESS on success, error code on failure
 */
int push_to_mirrors(struct repo_info *repo, const char *branch, int force);

/**
 * @brief Fetch latest changes from all mirrors
 * @param repo Repository information
 * @param merge_strategy Strategy for handling conflicts
 * @return SYNC_SUCCESS on success, error code on failure
 */
int fetch_from_mirrors(struct repo_info *repo, const char *merge_strategy);

/**
 * @brief Configure automatic synchronization
 * @param config Synchronization configuration
 * @param enable Enable or disable auto-sync
 * @param interval_hours Sync interval in hours
 * @return SYNC_SUCCESS on success, error code on failure
 */
int configure_auto_sync(struct sync_config *config, int enable, int interval_hours);

/**
 * @brief Load synchronization configuration from files
 * @param config Output structure for configuration
 * @return SYNC_SUCCESS on success, error code on failure
 */
int load_sync_config(struct sync_config *config);

/**
 * @brief Save synchronization configuration to file
 * @param config Configuration to save
 * @return SYNC_SUCCESS on success, error code on failure
 */
int save_sync_config(const struct sync_config *config);

/**
 * @brief Clone from best available mirror
 * @param url Primary repository URL
 * @param target_path Local path for clone
 * @param strategy Clone strategy to use
 * @param fallback_mirrors Array of fallback mirror URLs
 * @param fallback_count Number of fallback mirrors
 * @return SYNC_SUCCESS on success, error code on failure
 */
int clone_from_best_mirror(const char *url, const char *target_path,
                          enum clone_strategy strategy, const char **fallback_mirrors,
                          int fallback_count);

/**
 * @brief Monitor sync operations and report status
 * @param callback Function to call with status updates
 * @param user_data User data to pass to callback
 * @return SYNC_SUCCESS on success, error code on failure
 */
int monitor_sync_operations(void (*callback)(const char *status, void *user_data),
                           void *user_data);

/**
 * @brief Clean up remote mirror list
 * @param mirrors Mirror list to clean up
 */
void cleanup_remote_mirrors(struct remote_mirror *mirrors);

/**
 * @brief Clean up sync result structure
 * @param result Sync result to clean up
 */
void cleanup_sync_result(struct sync_result *result);

/**
 * @brief Clean up sync configuration
 * @param config Sync configuration to clean up
 */
void cleanup_sync_config(struct sync_config *config);

/**
 * @brief Get human-readable error message for sync error code
 * @param error_code Sync error code
 * @return Error message string
 */
const char* sync_get_error_string(int error_code);

/**
 * @brief Check if repository needs synchronization
 * @param repo Repository information
 * @param mirror_name Specific mirror to check (NULL for any)
 * @return 1 if sync needed, 0 if not, negative on error
 */
int needs_synchronization(const struct repo_info *repo, const char *mirror_name);

/**
 * @brief Update mirror sync status
 * @param repo Repository information
 * @param mirror_name Mirror name
 * @param status Sync status code
 * @param error_message Error message (NULL if successful)
 * @return SYNC_SUCCESS on success, error code on failure
 */
int update_mirror_sync_status(struct repo_info *repo, const char *mirror_name,
                             int status, const char *error_message);

#endif /* REMOTE_SYNC_H */