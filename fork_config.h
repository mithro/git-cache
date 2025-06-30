#ifndef FORK_CONFIG_H
#define FORK_CONFIG_H

/**
 * @file fork_config.h
 * @brief Fork repository configuration and management for git-cache
 * 
 * This module provides functionality for configuring how git-cache handles
 * repository forking, including organization settings, privacy preferences,
 * and fork synchronization options.
 */

#include "git-cache.h"

/* Forward declarations */
struct repo_info;
struct cache_config;
struct github_client;

/**
 * @brief Fork configuration settings
 */
struct fork_config {
	char *default_organization;   /**< Default organization for forks */
	int auto_fork;               /**< Automatically fork when needed */
	int fork_private_as_private; /**< Keep private repos private when forking */
	int fork_public_as_private;  /**< Make public repos private when forking */
	int sync_with_upstream;      /**< Keep forks synchronized with upstream */
	int sync_interval_hours;     /**< Sync interval with upstream */
	int delete_branch_on_merge;  /**< Delete branch after PR merge */
	int allow_force_push;        /**< Allow force push to fork branches */
	char *branch_prefix;         /**< Prefix for fork branches */
};

/**
 * @brief Fork synchronization status
 */
struct fork_sync_status {
	char *fork_url;              /**< Fork repository URL */
	char *upstream_url;          /**< Upstream repository URL */
	int commits_behind;          /**< Number of commits behind upstream */
	int commits_ahead;           /**< Number of commits ahead of upstream */
	time_t last_sync;           /**< Last synchronization time */
	int has_conflicts;          /**< Whether merge conflicts exist */
};

/**
 * @brief Fork operation result
 */
struct fork_result {
	int success;                /**< Whether fork succeeded */
	char *fork_url;            /**< URL of created fork */
	char *error_message;       /**< Error message if failed */
	int already_exists;        /**< Fork already existed */
};

/**
 * @brief Load fork configuration from files and environment
 * @param config Output structure for configuration
 * @return 0 on success, negative error code on failure
 */
int load_fork_config(struct fork_config *config);

/**
 * @brief Save fork configuration to file
 * @param config Configuration to save
 * @return 0 on success, negative error code on failure
 */
int save_fork_config(const struct fork_config *config);

/**
 * @brief Configure repository for forking
 * @param repo Repository information
 * @param config Fork configuration settings
 * @return 0 on success, negative error code on failure
 */
int configure_fork_settings(struct repo_info *repo, const struct fork_config *config);

/**
 * @brief Create fork with specified settings
 * @param client GitHub API client
 * @param owner Original repository owner
 * @param name Repository name
 * @param config Fork configuration
 * @param result Output structure for fork result
 * @return 0 on success, negative error code on failure
 */
int create_fork_with_config(struct github_client *client, const char *owner,
	                       const char *name, const struct fork_config *config,
	                       struct fork_result *result);

/**
 * @brief Check if repository needs forking
 * @param repo Repository information
 * @param config Fork configuration
 * @return 1 if fork needed, 0 if not, negative on error
 */
int needs_fork(const struct repo_info *repo, const struct fork_config *config);

/**
 * @brief Synchronize fork with upstream
 * @param repo Repository information (must have fork_url set)
 * @param branch Specific branch to sync (NULL for default)
 * @param force Force sync even with local changes
 * @return 0 on success, negative error code on failure
 */
int sync_fork_with_upstream(const struct repo_info *repo, const char *branch, int force);

/**
 * @brief Get fork synchronization status
 * @param repo Repository information
 * @param status Output structure for sync status
 * @return 0 on success, negative error code on failure
 */
int get_fork_sync_status(const struct repo_info *repo, struct fork_sync_status *status);

/**
 * @brief Configure fork remote in local repository
 * @param repo_path Path to local repository
 * @param fork_url URL of fork repository
 * @param upstream_url URL of upstream repository
 * @return 0 on success, negative error code on failure
 */
int configure_fork_remotes(const char *repo_path, const char *fork_url,
	                      const char *upstream_url);

/**
 * @brief Set default fork organization
 * @param config Fork configuration
 * @param organization Organization name (NULL to unset)
 * @return 0 on success, negative error code on failure
 */
int set_default_fork_organization(struct fork_config *config, const char *organization);

/**
 * @brief Apply fork privacy settings based on configuration
 * @param client GitHub API client
 * @param fork_owner Fork owner
 * @param fork_name Fork repository name
 * @param original_private Whether original repo was private
 * @param config Fork configuration
 * @return 0 on success, negative error code on failure
 */
int apply_fork_privacy_settings(struct github_client *client, const char *fork_owner,
	                           const char *fork_name, int original_private,
	                           const struct fork_config *config);

/**
 * @brief List all forks managed by git-cache
 * @param config Cache configuration
 * @param callback Function to call for each fork
 * @param user_data User data to pass to callback
 * @return Number of forks found, or negative error code
 */
int list_managed_forks(const struct cache_config *config,
	                  int (*callback)(const struct repo_info *repo, void *user_data),
	                  void *user_data);

/**
 * @brief Delete fork and clean up references
 * @param client GitHub API client
 * @param repo Repository information with fork
 * @param delete_remote Also delete remote repository
 * @return 0 on success, negative error code on failure
 */
int delete_fork(struct github_client *client, struct repo_info *repo, int delete_remote);

/**
 * @brief Get default fork configuration
 * @param config Output structure for default configuration
 */
void get_default_fork_config(struct fork_config *config);

/**
 * @brief Clean up fork configuration structure
 * @param config Fork configuration to clean up
 */
void cleanup_fork_config(struct fork_config *config);

/**
 * @brief Clean up fork result structure
 * @param result Fork result to clean up
 */
void cleanup_fork_result(struct fork_result *result);

/**
 * @brief Clean up fork sync status structure
 * @param status Fork sync status to clean up
 */
void cleanup_fork_sync_status(struct fork_sync_status *status);

/**
 * @brief Check if user has permission to push to repository
 * @param client GitHub API client
 * @param owner Repository owner
 * @param name Repository name
 * @return 1 if can push, 0 if not, negative on error
 */
int can_push_to_repository(struct github_client *client, const char *owner,
	                      const char *name);

/**
 * @brief Create pull request from fork to upstream
 * @param client GitHub API client
 * @param repo Repository information with fork
 * @param branch Branch name in fork
 * @param title Pull request title
 * @param body Pull request body
 * @return PR number on success, negative error code on failure
 */
int create_pull_request_from_fork(struct github_client *client, const struct repo_info *repo,
	                             const char *branch, const char *title, const char *body);

#endif /* FORK_CONFIG_H */