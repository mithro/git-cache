#ifndef CHECKOUT_REPAIR_H
#define CHECKOUT_REPAIR_H

/**
 * @file checkout_repair.h
 * @brief Checkout repair mechanisms for git-cache
 * 
 * Provides functionality for detecting and repairing checkouts that have
 * become outdated or corrupted when the cache is updated.
 */

#include "git-cache.h"
#include <time.h>

/* Forward declarations */
struct repo_info;
struct cache_config;
struct cache_metadata;

/**
 * @brief Checkout repair status codes
 */
#define CHECKOUT_REPAIR_SUCCESS         0
#define CHECKOUT_REPAIR_NOT_NEEDED     1
#define CHECKOUT_REPAIR_FAILED        -1
#define CHECKOUT_REPAIR_INVALID_ARGS  -2
#define CHECKOUT_REPAIR_NO_CACHE      -3
#define CHECKOUT_REPAIR_IO_ERROR      -4

/**
 * @brief Check if checkout needs repair due to cache updates
 * @param checkout_path Path to checkout directory
 * @param cache_path Path to cache repository
 * @param last_cache_sync Time when cache was last synchronized
 * @return 1 if repair needed, 0 if not needed, negative on error
 */
int checkout_needs_repair(const char *checkout_path, const char *cache_path, time_t last_cache_sync);

/**
 * @brief Repair a checkout that has become outdated
 * @param checkout_path Path to checkout directory
 * @param cache_path Path to cache repository
 * @param strategy Clone strategy to use for repair
 * @param verbose Enable verbose output
 * @return CHECKOUT_REPAIR_SUCCESS on success, error code on failure
 */
int repair_outdated_checkout(const char *checkout_path, const char *cache_path, 
	                        enum clone_strategy strategy, int verbose);

/**
 * @brief Check and repair all checkouts for a repository
 * @param repo Repository information
 * @param config Cache configuration
 * @return Number of checkouts repaired, or negative error code
 */
int repair_all_checkouts_for_repo(struct repo_info *repo, const struct cache_config *config);

/**
 * @brief Find and repair all outdated checkouts in the system
 * @param config Cache configuration
 * @param force_repair Force repair even if not obviously needed
 * @return Number of checkouts repaired, or negative error code
 */
int repair_all_outdated_checkouts(struct cache_config *config, int force_repair);

/**
 * @brief Check if checkout references are valid and point to correct cache
 * @param checkout_path Path to checkout directory
 * @param expected_cache_path Expected cache path
 * @return 1 if valid, 0 if invalid, negative on error
 */
int validate_checkout_references(const char *checkout_path, const char *expected_cache_path);

/**
 * @brief Update checkout to latest state from cache
 * @param checkout_path Path to checkout directory
 * @param cache_path Path to cache repository
 * @param verbose Enable verbose output
 * @return CHECKOUT_REPAIR_SUCCESS on success, error code on failure
 */
int update_checkout_from_cache(const char *checkout_path, const char *cache_path, int verbose);

/**
 * @brief Detect orphaned checkouts (cache no longer exists)
 * @param config Cache configuration
 * @param callback Function to call for each orphaned checkout
 * @param user_data User data to pass to callback
 * @return Number of orphaned checkouts found, or negative error code
 */
int detect_orphaned_checkouts(struct cache_config *config,
	                         int (*callback)(const char *checkout_path, void *user_data),
	                         void *user_data);

/**
 * @brief Check if cache has been updated since checkout was created
 * @param checkout_path Path to checkout directory
 * @param cache_path Path to cache repository
 * @param checkout_mtime Output: modification time of checkout
 * @param cache_mtime Output: modification time of cache
 * @return 1 if cache is newer, 0 if not, negative on error
 */
int cache_newer_than_checkout(const char *checkout_path, const char *cache_path,
	                         time_t *checkout_mtime, time_t *cache_mtime);

/**
 * @brief Get checkout repair status message
 * @param status_code Repair status code
 * @return Human-readable status message
 */
const char* checkout_repair_status_string(int status_code);

#endif /* CHECKOUT_REPAIR_H */