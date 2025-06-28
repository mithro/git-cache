#ifndef CACHE_RECOVERY_H
#define CACHE_RECOVERY_H

/**
 * @file cache_recovery.h
 * @brief Cache corruption detection and recovery for git-cache
 * 
 * Provides functions for detecting and recovering from corrupted
 * cache repositories and checkout directories.
 */

#include "git-cache.h"

/* Forward declarations */
struct repo_info;
struct cache_config;

/**
 * @brief Recovery status codes
 */
#define CACHE_RECOVERY_OK               0
#define CACHE_RECOVERY_NOT_EXISTS      -1
#define CACHE_RECOVERY_NOT_GIT_REPO    -2
#define CACHE_RECOVERY_CORRUPTED       -3
#define CACHE_RECOVERY_MISSING_REFS    -4
#define CACHE_RECOVERY_EMPTY_REPO      -5
#define CACHE_RECOVERY_NO_ALTERNATES   -6
#define CACHE_RECOVERY_WRONG_ALTERNATES -7
#define CACHE_RECOVERY_INVALID_PATH    -8
#define CACHE_RECOVERY_REPAIR_FAILED   -9

/**
 * @brief Check if a Git repository is valid and not corrupted
 * @param repo_path Path to the repository
 * @return CACHE_RECOVERY_OK if valid, error code otherwise
 */
int verify_git_repository(const char *repo_path);

/**
 * @brief Verify cache repository integrity
 * @param cache_path Path to the cache repository
 * @return CACHE_RECOVERY_OK if valid, error code otherwise
 */
int verify_cache_repository(const char *cache_path);

/**
 * @brief Verify checkout repository integrity
 * @param checkout_path Path to the checkout repository
 * @param cache_path Path to the cache repository it should reference
 * @return CACHE_RECOVERY_OK if valid, error code otherwise
 */
int verify_checkout_repository(const char *checkout_path, const char *cache_path);

/**
 * @brief Attempt to repair a corrupted cache repository
 * @param cache_path Path to the corrupted cache
 * @param original_url Original repository URL for re-cloning
 * @param verbose Enable verbose output
 * @return CACHE_RECOVERY_OK if repaired, error code otherwise
 */
int repair_cache_repository(const char *cache_path, const char *original_url, int verbose);

/**
 * @brief Repair a checkout repository by recreating it from cache
 * @param checkout_path Path to the checkout to repair
 * @param cache_path Path to the cache repository
 * @param strategy Clone strategy to use for recreation
 * @param verbose Enable verbose output
 * @return CACHE_RECOVERY_OK if repaired, error code otherwise
 */
int repair_checkout_repository(const char *checkout_path, const char *cache_path, 
	                          enum clone_strategy strategy, int verbose);

/**
 * @brief Comprehensive verification and repair of repository and checkouts
 * @param repo Repository information
 * @param config Cache configuration
 * @return CACHE_RECOVERY_OK if all valid/repaired, error code otherwise
 */
int verify_and_repair_repository(struct repo_info *repo, struct cache_config *config);

/**
 * @brief Get human-readable error message for recovery status
 * @param error_code Recovery error code
 * @return Error message string
 */
const char* cache_recovery_error_string(int error_code);

#endif /* CACHE_RECOVERY_H */