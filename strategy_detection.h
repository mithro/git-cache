#ifndef STRATEGY_DETECTION_H
#define STRATEGY_DETECTION_H

/**
 * @file strategy_detection.h
 * @brief Auto-detection of optimal clone strategies for git-cache
 * 
 * This module analyzes repository characteristics and automatically
 * selects the best clone strategy based on size, history, and usage patterns.
 */

#include "git-cache.h"
#include <stdint.h>

/* Forward declarations */
struct repo_info;
struct cache_config;

/**
 * @brief Repository analysis results
 */
struct repo_analysis {
	uint64_t estimated_size;    /**< Estimated repository size in bytes */
	int commit_count;           /**< Number of commits in default branch */
	int branch_count;           /**< Number of branches */
	int tag_count;             /**< Number of tags */
	int file_count;            /**< Number of files in repository */
	int has_large_files;       /**< Whether repository contains large files */
	int has_binaries;          /**< Whether repository contains binary files */
	int is_monorepo;           /**< Whether this appears to be a monorepo */
	int activity_level;        /**< Recent activity level (0-100) */
	char *primary_language;    /**< Primary programming language */
	time_t last_activity;      /**< Last commit timestamp */
};

/**
 * @brief Strategy recommendation with confidence
 */
struct strategy_recommendation {
	enum clone_strategy strategy;  /**< Recommended strategy */
	int confidence;               /**< Confidence level (0-100) */
	char *reasoning;              /**< Human-readable reasoning */
	enum clone_strategy fallback; /**< Fallback strategy if primary fails */
};

/**
 * @brief Strategy detection configuration
 */
struct strategy_config {
	int prefer_speed;        /**< Prefer faster clone over completeness */
	int prefer_completeness; /**< Prefer complete history over speed */
	uint64_t size_threshold_mb; /**< Size threshold for strategy switching */
	int depth_threshold;     /**< Commit count threshold for shallow clones */
	int enable_filters;      /**< Enable partial clone filters */
	int respect_user_pref;   /**< Respect user's previous strategy choices */
};

/**
 * @brief Analyze repository characteristics from URL
 * @param url Repository URL to analyze
 * @param analysis Output structure for analysis results
 * @return 0 on success, negative error code on failure
 */
int analyze_repository_from_url(const char *url, struct repo_analysis *analysis);

/**
 * @brief Analyze repository characteristics from local path
 * @param repo_path Path to local repository
 * @param analysis Output structure for analysis results
 * @return 0 on success, negative error code on failure
 */
int analyze_repository_from_path(const char *repo_path, struct repo_analysis *analysis);

/**
 * @brief Get optimal clone strategy based on analysis
 * @param analysis Repository analysis results
 * @param config Strategy detection configuration
 * @param recommendation Output structure for strategy recommendation
 * @return 0 on success, negative error code on failure
 */
int get_optimal_strategy(const struct repo_analysis *analysis,
	                    const struct strategy_config *config,
	                    struct strategy_recommendation *recommendation);

/**
 * @brief Auto-detect and set optimal strategy for repository
 * @param repo Repository information (strategy will be updated)
 * @param config Cache configuration
 * @return 0 on success, negative error code on failure
 */
int auto_detect_strategy(struct repo_info *repo, const struct cache_config *config);

/**
 * @brief Learn from user's strategy choices to improve recommendations
 * @param repo Repository information with user's chosen strategy
 * @param analysis Repository analysis that led to the choice
 * @param success Whether the chosen strategy worked well
 * @return 0 on success, negative error code on failure
 */
int learn_from_strategy_choice(const struct repo_info *repo,
	                          const struct repo_analysis *analysis,
	                          int success);

/**
 * @brief Get default strategy configuration
 * @param config Output structure for default configuration
 */
void get_default_strategy_config(struct strategy_config *config);

/**
 * @brief Load strategy configuration from file or environment
 * @param config Output structure for loaded configuration
 * @return 0 on success, negative error code on failure
 */
int load_strategy_config(struct strategy_config *config);

/**
 * @brief Clean up repository analysis structure
 * @param analysis Analysis structure to clean up
 */
void cleanup_repo_analysis(struct repo_analysis *analysis);

/**
 * @brief Clean up strategy recommendation structure
 * @param recommendation Recommendation structure to clean up
 */
void cleanup_strategy_recommendation(struct strategy_recommendation *recommendation);

/**
 * @brief Get human-readable description of clone strategy
 * @param strategy Clone strategy
 * @return Human-readable description
 */
const char* get_strategy_description(enum clone_strategy strategy);

/**
 * @brief Estimate download time for different strategies
 * @param analysis Repository analysis
 * @param strategy Clone strategy to estimate
 * @param bandwidth_mbps Estimated bandwidth in Mbps
 * @return Estimated download time in seconds
 */
int estimate_download_time(const struct repo_analysis *analysis,
	                      enum clone_strategy strategy,
	                      int bandwidth_mbps);

/**
 * @brief Check if repository supports partial clone
 * @param url Repository URL
 * @return 1 if supported, 0 if not, negative on error
 */
int supports_partial_clone(const char *url);

#endif /* STRATEGY_DETECTION_H */