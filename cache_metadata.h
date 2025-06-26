#ifndef CACHE_METADATA_H
#define CACHE_METADATA_H

/**
 * @file cache_metadata.h
 * @brief Cache metadata storage and retrieval for git-cache
 * 
 * This module provides functionality for storing and retrieving metadata
 * about cached repositories, including clone strategies, timestamps,
 * and repository information.
 */

#include "git-cache.h"
#include <time.h>

/* Forward declarations */
struct repo_info;
struct cache_config;

/**
 * @brief Cache metadata structure
 * 
 * Contains all metadata about a cached repository that needs to be
 * persisted across sessions.
 */
struct cache_metadata {
	char *original_url;        /**< Original repository URL */
	char *fork_url;           /**< Fork repository URL (if applicable) */
	char *owner;              /**< Repository owner */
	char *name;               /**< Repository name */
	char *fork_organization;  /**< Organization used for fork */
	enum repo_type type;      /**< Repository type */
	enum clone_strategy strategy; /**< Clone strategy used */
	time_t created_time;      /**< When cache was created */
	time_t last_sync_time;    /**< Last synchronization time */
	time_t last_access_time;  /**< Last access time */
	int is_fork_needed;       /**< Whether forking was needed */
	int is_private_fork;      /**< Whether fork is private */
	int has_submodules;       /**< Whether repository has submodules */
	char *default_branch;     /**< Default branch name */
	size_t cache_size;        /**< Cache size in bytes */
	int ref_count;            /**< Number of active checkouts */
};

/**
 * @brief Metadata storage error codes
 */
#define METADATA_SUCCESS           0
#define METADATA_ERROR_NOT_FOUND  -1
#define METADATA_ERROR_INVALID    -2
#define METADATA_ERROR_IO         -3
#define METADATA_ERROR_MEMORY     -4
#define METADATA_ERROR_CORRUPT    -5

/**
 * @brief Create metadata structure from repository info
 * @param repo Repository information
 * @return New metadata structure or NULL on failure
 */
struct cache_metadata* cache_metadata_create(const struct repo_info *repo);

/**
 * @brief Destroy metadata structure
 * @param metadata Metadata to destroy
 */
void cache_metadata_destroy(struct cache_metadata *metadata);

/**
 * @brief Save metadata to storage
 * @param cache_path Path to cache directory
 * @param metadata Metadata to save
 * @return METADATA_SUCCESS on success, error code on failure
 */
int cache_metadata_save(const char *cache_path, const struct cache_metadata *metadata);

/**
 * @brief Load metadata from storage
 * @param cache_path Path to cache directory
 * @param metadata Metadata structure to populate
 * @return METADATA_SUCCESS on success, error code on failure
 */
int cache_metadata_load(const char *cache_path, struct cache_metadata *metadata);

/**
 * @brief Update last access time
 * @param cache_path Path to cache directory
 * @return METADATA_SUCCESS on success, error code on failure
 */
int cache_metadata_update_access(const char *cache_path);

/**
 * @brief Update last sync time
 * @param cache_path Path to cache directory
 * @return METADATA_SUCCESS on success, error code on failure
 */
int cache_metadata_update_sync(const char *cache_path);

/**
 * @brief Increment reference count (active checkouts)
 * @param cache_path Path to cache directory
 * @return METADATA_SUCCESS on success, error code on failure
 */
int cache_metadata_increment_ref(const char *cache_path);

/**
 * @brief Decrement reference count (active checkouts)
 * @param cache_path Path to cache directory
 * @return METADATA_SUCCESS on success, error code on failure
 */
int cache_metadata_decrement_ref(const char *cache_path);

/**
 * @brief Get repository metadata by URL
 * @param config Cache configuration
 * @param url Repository URL
 * @param metadata Metadata structure to populate
 * @return METADATA_SUCCESS on success, error code on failure
 */
int cache_metadata_get_by_url(const struct cache_config *config, const char *url, 
                             struct cache_metadata *metadata);

/**
 * @brief List all cached repositories with metadata
 * @param config Cache configuration
 * @param callback Function to call for each repository
 * @param user_data User data to pass to callback
 * @return Number of repositories found, or negative error code
 */
int cache_metadata_list_all(const struct cache_config *config,
                           int (*callback)(const struct cache_metadata *metadata, void *user_data),
                           void *user_data);

/**
 * @brief Calculate cache directory size
 * @param cache_path Path to cache directory
 * @return Size in bytes, or 0 on error
 */
size_t cache_metadata_calculate_size(const char *cache_path);

/**
 * @brief Check if metadata exists for cache
 * @param cache_path Path to cache directory
 * @return 1 if exists, 0 if not, -1 on error
 */
int cache_metadata_exists(const char *cache_path);

/**
 * @brief Get metadata file path for cache
 * @param cache_path Path to cache directory
 * @param metadata_path Buffer to store metadata file path
 * @param path_size Size of buffer
 * @return METADATA_SUCCESS on success, error code on failure
 */
int cache_metadata_get_path(const char *cache_path, char *metadata_path, size_t path_size);

#endif /* CACHE_METADATA_H */