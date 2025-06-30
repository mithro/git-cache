/**
 * @file strategy_detection.c
 * @brief Implementation of automatic clone strategy detection
 * 
 * Analyzes repository characteristics and automatically selects the best
 * clone strategy based on size, history, and usage patterns.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

#include "git-cache.h"
#include "strategy_detection.h"
#include "github_api.h"

/* Size thresholds in MB */
#define SMALL_REPO_THRESHOLD_MB    10
#define MEDIUM_REPO_THRESHOLD_MB   100
#define LARGE_REPO_THRESHOLD_MB    500
#define HUGE_REPO_THRESHOLD_MB     2000

/* Commit count thresholds */
#define SHALLOW_COMMIT_THRESHOLD   100
#define DEEP_HISTORY_THRESHOLD     10000
#define MASSIVE_HISTORY_THRESHOLD  50000

/* Activity thresholds (commits per month) */
#define LOW_ACTIVITY_THRESHOLD     5
#define HIGH_ACTIVITY_THRESHOLD    50

/**
 * @brief Get default strategy configuration
 */
void get_default_strategy_config(struct strategy_config *config)
{
	if (!config) {
		return;
	}
	
	config->prefer_speed = 1;
	config->prefer_completeness = 0;
	config->size_threshold_mb = MEDIUM_REPO_THRESHOLD_MB;
	config->depth_threshold = SHALLOW_COMMIT_THRESHOLD;
	config->enable_filters = 1;
	config->respect_user_pref = 1;
}

/**
 * @brief Load strategy configuration from environment variables
 */
int load_strategy_config(struct strategy_config *config)
{
	if (!config) {
		return -1;
	}
	
	/* Start with defaults */
	get_default_strategy_config(config);
	
	/* Override with environment variables */
	const char *prefer_speed = getenv("GIT_CACHE_PREFER_SPEED");
	if (prefer_speed && strcmp(prefer_speed, "1") == 0) {
		config->prefer_speed = 1;
		config->prefer_completeness = 0;
	}
	
	const char *prefer_complete = getenv("GIT_CACHE_PREFER_COMPLETE");
	if (prefer_complete && strcmp(prefer_complete, "1") == 0) {
		config->prefer_completeness = 1;
		config->prefer_speed = 0;
	}
	
	const char *size_threshold = getenv("GIT_CACHE_SIZE_THRESHOLD_MB");
	if (size_threshold) {
		int threshold = atoi(size_threshold);
		if (threshold > 0) {
			config->size_threshold_mb = threshold;
		}
	}
	
	const char *depth_threshold = getenv("GIT_CACHE_DEPTH_THRESHOLD");
	if (depth_threshold) {
		int threshold = atoi(depth_threshold);
		if (threshold > 0) {
			config->depth_threshold = threshold;
		}
	}
	
	return 0;
}

/**
 * @brief Extract repository info from GitHub API (simplified version)
 */
static int analyze_github_repository(const char *owner, const char *name, 
	                                struct repo_analysis *analysis)
{
	struct github_client *client = github_client_create(NULL);
	if (!client) {
		return -1;
	}
	
	struct github_repo *repo_info = NULL;
	int ret = github_get_repo(client, owner, name, &repo_info);
	if (ret != 0 || !repo_info) {
		github_client_destroy(client);
		return -1;
	}
	
	/* Use basic repository characteristics for analysis */
	/* Since we don't have size info from this API, use conservative estimates */
	analysis->estimated_size = MEDIUM_REPO_THRESHOLD_MB * 1024 * 1024; /* Default to medium */
	analysis->has_large_files = 0; /* Conservative default */
	analysis->last_activity = time(NULL); /* Current time as fallback */
	analysis->activity_level = 50; /* Medium activity */
	
	/* Check if it's a fork - forks are often smaller */
	if (repo_info->is_fork) {
		analysis->estimated_size = SMALL_REPO_THRESHOLD_MB * 1024 * 1024;
		analysis->activity_level = 30;
	}
	
	/* Popular repositories (many forks) might be larger */
	if (repo_info->fork_count > 100) {
		analysis->estimated_size = LARGE_REPO_THRESHOLD_MB * 1024 * 1024;
		analysis->activity_level = 70;
		analysis->is_monorepo = 1;
	}
	
	/* Set reasonable defaults */
	analysis->primary_language = strdup("unknown");
	
	github_repo_destroy(repo_info);
	github_client_destroy(client);
	
	return 0;
}

/**
 * @brief Analyze repository from URL using available APIs
 */
int analyze_repository_from_url(const char *url, struct repo_analysis *analysis)
{
	if (!url || !analysis) {
		return -1;
	}
	
	/* Initialize analysis structure */
	memset(analysis, 0, sizeof(*analysis));
	
	/* Try to extract GitHub repository information */
	if (strstr(url, "github.com")) {
		/* Parse GitHub URL to get owner/repo */
		char *url_copy = strdup(url);
		if (!url_copy) {
			return -1;
		}
		char *github_part = strstr(url_copy, "github.com/");
		if (github_part) {
			github_part += strlen("github.com/");
			char *owner = github_part;
			char *slash = strchr(owner, '/');
			if (slash) {
				*slash = '\0';
				char *repo = slash + 1;
				
				/* Remove .git suffix if present */
				char *dot_git = strstr(repo, ".git");
				if (dot_git) {
					*dot_git = '\0';
				}
				
				/* Analyze using GitHub API */
				if (analyze_github_repository(owner, repo, analysis) == 0) {
					free(url_copy);
					return 0;
				}
			}
		}
		free(url_copy);
	}
	
	/* Fallback: Set conservative defaults */
	analysis->estimated_size = MEDIUM_REPO_THRESHOLD_MB * 1024 * 1024;
	analysis->commit_count = 1000;
	analysis->branch_count = 5;
	analysis->tag_count = 10;
	analysis->file_count = 500;
	analysis->activity_level = 50;
	analysis->primary_language = strdup("unknown");
	
	return 0;
}

/**
 * @brief Analyze repository from local path
 */
int analyze_repository_from_path(const char *repo_path, struct repo_analysis *analysis)
{
	if (!repo_path || !analysis) {
		return -1;
	}
	
	/* Initialize analysis structure */
	memset(analysis, 0, sizeof(*analysis));
	
	/* Check if path exists and is a git repository */
	char git_dir[4096];
	snprintf(git_dir, sizeof(git_dir), "%s/.git", repo_path);
	if (access(git_dir, F_OK) != 0) {
		/* Try bare repository */
		snprintf(git_dir, sizeof(git_dir), "%s/refs", repo_path);
		if (access(git_dir, F_OK) != 0) {
			return -1;
		}
	}
	
	/* Get repository size */
	char size_cmd[4096];
	snprintf(size_cmd, sizeof(size_cmd), "du -sb \"%s\" 2>/dev/null | cut -f1", repo_path);
	
	FILE *pipe = popen(size_cmd, "r");
	if (pipe) {
		if (fscanf(pipe, "%lu", &analysis->estimated_size) != 1) {
			analysis->estimated_size = 0;
		}
		pclose(pipe);
	}
	
	/* Count commits */
	char commit_cmd[4096];
	snprintf(commit_cmd, sizeof(commit_cmd), 
	         "cd \"%s\" && git rev-list --count HEAD 2>/dev/null", repo_path);
	
	pipe = popen(commit_cmd, "r");
	if (pipe) {
		if (fscanf(pipe, "%d", &analysis->commit_count) != 1) {
			analysis->commit_count = 0;
		}
		pclose(pipe);
	}
	
	/* Count branches */
	char branch_cmd[4096];
	snprintf(branch_cmd, sizeof(branch_cmd), 
	         "cd \"%s\" && git branch -r 2>/dev/null | wc -l", repo_path);
	
	pipe = popen(branch_cmd, "r");
	if (pipe) {
		if (fscanf(pipe, "%d", &analysis->branch_count) != 1) {
			analysis->branch_count = 1;
		}
		pclose(pipe);
	}
	
	/* Count tags */
	char tag_cmd[4096];
	snprintf(tag_cmd, sizeof(tag_cmd), 
	         "cd \"%s\" && git tag 2>/dev/null | wc -l", repo_path);
	
	pipe = popen(tag_cmd, "r");
	if (pipe) {
		if (fscanf(pipe, "%d", &analysis->tag_count) != 1) {
			analysis->tag_count = 0;
		}
		pclose(pipe);
	}
	
	/* Get last activity */
	char activity_cmd[4096];
	snprintf(activity_cmd, sizeof(activity_cmd), 
	         "cd \"%s\" && git log -1 --format=%%ct 2>/dev/null", repo_path);
	
	pipe = popen(activity_cmd, "r");
	if (pipe) {
		if (fscanf(pipe, "%ld", &analysis->last_activity) != 1) {
			analysis->last_activity = time(NULL);
		}
		pclose(pipe);
	}
	
	/* Determine activity level */
	time_t now = time(NULL);
	int days_since_update = (now - analysis->last_activity) / (24 * 60 * 60);
	
	if (days_since_update < 7) {
		analysis->activity_level = 90;
	} else if (days_since_update < 30) {
		analysis->activity_level = 70;
	} else if (days_since_update < 90) {
		analysis->activity_level = 50;
	} else {
		analysis->activity_level = 30;
	}
	
	/* Check for large files (>10MB) */
	char large_files_cmd[4096];
	snprintf(large_files_cmd, sizeof(large_files_cmd),
	         "cd \"%s\" && find . -type f -size +10M 2>/dev/null | head -1", repo_path);
	
	pipe = popen(large_files_cmd, "r");
	if (pipe) {
		char buffer[256];
		analysis->has_large_files = (fgets(buffer, sizeof(buffer), pipe) != NULL);
		pclose(pipe);
	}
	
	/* Estimate if monorepo based on size and file count */
	analysis->is_monorepo = (analysis->estimated_size > (500 * 1024 * 1024)) ||
	                       (analysis->commit_count > 10000);
	
	return 0;
}

/**
 * @brief Get optimal clone strategy based on analysis
 */
int get_optimal_strategy(const struct repo_analysis *analysis,
	                    const struct strategy_config *config,
	                    struct strategy_recommendation *recommendation)
{
	if (!analysis || !config || !recommendation) {
		return -1;
	}
	
	/* Initialize recommendation */
	memset(recommendation, 0, sizeof(*recommendation));
	recommendation->fallback = CLONE_STRATEGY_FULL;
	
	uint64_t size_mb = analysis->estimated_size / (1024 * 1024);
	
	/* Decision logic based on repository characteristics */
	
	/* Very small repositories: always full clone */
	if (size_mb < SMALL_REPO_THRESHOLD_MB && analysis->commit_count < 100) {
		recommendation->strategy = CLONE_STRATEGY_FULL;
		recommendation->confidence = 95;
		recommendation->reasoning = strdup("Small repository - full clone is optimal");
		recommendation->fallback = CLONE_STRATEGY_SHALLOW;
		return 0;
	}
	
	/* Large repositories with deep history: use filters */
	if (size_mb > config->size_threshold_mb || analysis->commit_count > config->depth_threshold) {
		if (config->prefer_speed) {
			if (analysis->has_large_files || analysis->is_monorepo) {
				recommendation->strategy = CLONE_STRATEGY_BLOBLESS;
				recommendation->confidence = 85;
				recommendation->reasoning = strdup("Large repository with binary files - blobless clone recommended");
				recommendation->fallback = CLONE_STRATEGY_TREELESS;
			} else if (size_mb > LARGE_REPO_THRESHOLD_MB) {
				recommendation->strategy = CLONE_STRATEGY_TREELESS;
				recommendation->confidence = 80;
				recommendation->reasoning = strdup("Large repository - treeless clone for faster download");
				recommendation->fallback = CLONE_STRATEGY_BLOBLESS;
			} else {
				recommendation->strategy = CLONE_STRATEGY_SHALLOW;
				recommendation->confidence = 75;
				recommendation->reasoning = strdup("Medium repository - shallow clone for speed");
				recommendation->fallback = CLONE_STRATEGY_TREELESS;
			}
		} else {
			/* Prefer completeness */
			recommendation->strategy = CLONE_STRATEGY_FULL;
			recommendation->confidence = 70;
			recommendation->reasoning = strdup("Full history preferred despite size");
			recommendation->fallback = CLONE_STRATEGY_TREELESS;
		}
		return 0;
	}
	
	/* Very active repositories: consider shallow for quick updates */
	if (analysis->activity_level > HIGH_ACTIVITY_THRESHOLD && config->prefer_speed) {
		recommendation->strategy = CLONE_STRATEGY_SHALLOW;
		recommendation->confidence = 70;
		recommendation->reasoning = strdup("High activity repository - shallow clone for quick updates");
		recommendation->fallback = CLONE_STRATEGY_FULL;
		return 0;
	}
	
	/* Monorepos: almost always use partial clones */
	if (analysis->is_monorepo) {
		recommendation->strategy = CLONE_STRATEGY_BLOBLESS;
		recommendation->confidence = 90;
		recommendation->reasoning = strdup("Monorepo detected - blobless clone recommended");
		recommendation->fallback = CLONE_STRATEGY_TREELESS;
		return 0;
	}
	
	/* Archive repositories (low activity): full clone is fine */
	if (analysis->activity_level < LOW_ACTIVITY_THRESHOLD) {
		recommendation->strategy = CLONE_STRATEGY_FULL;
		recommendation->confidence = 80;
		recommendation->reasoning = strdup("Low activity repository - full clone appropriate");
		recommendation->fallback = CLONE_STRATEGY_SHALLOW;
		return 0;
	}
	
	/* Default: balanced approach */
	if (size_mb > MEDIUM_REPO_THRESHOLD_MB / 2) {
		recommendation->strategy = CLONE_STRATEGY_TREELESS;
		recommendation->confidence = 60;
		recommendation->reasoning = strdup("Medium-sized repository - treeless clone balances speed and functionality");
		recommendation->fallback = CLONE_STRATEGY_SHALLOW;
	} else {
		recommendation->strategy = CLONE_STRATEGY_FULL;
		recommendation->confidence = 65;
		recommendation->reasoning = strdup("Standard repository - full clone recommended");
		recommendation->fallback = CLONE_STRATEGY_SHALLOW;
	}
	
	return 0;
}

/**
 * @brief Auto-detect and set optimal strategy for repository
 */
int auto_detect_strategy(struct repo_info *repo, const struct cache_config *config)
{
	if (!repo || !config) {
		return -1;
	}
	
	/* Load strategy configuration */
	struct strategy_config strategy_config;
	load_strategy_config(&strategy_config);
	
	/* Analyze repository */
	struct repo_analysis analysis;
	int ret = analyze_repository_from_url(repo->original_url, &analysis);
	if (ret != 0) {
		cleanup_repo_analysis(&analysis);
		return ret;
	}
	
	/* Get strategy recommendation */
	struct strategy_recommendation recommendation;
	ret = get_optimal_strategy(&analysis, &strategy_config, &recommendation);
	if (ret != 0) {
		cleanup_repo_analysis(&analysis);
		return ret;
	}
	
	/* Apply recommendation if confidence is high enough */
	if (recommendation.confidence >= 70) {
		repo->strategy = recommendation.strategy;
		
		if (config->verbose) {
			printf("Auto-detected clone strategy: %s (confidence: %d%%)\n",
			       get_strategy_description(recommendation.strategy),
			       recommendation.confidence);
			if (recommendation.reasoning) {
				printf("Reasoning: %s\n", recommendation.reasoning);
			}
		}
	} else {
		/* Low confidence - use default */
		repo->strategy = config->default_strategy;
		
		if (config->verbose) {
			printf("Using default clone strategy: %s (low confidence: %d%%)\n",
			       get_strategy_description(repo->strategy),
			       recommendation.confidence);
		}
	}
	
	cleanup_repo_analysis(&analysis);
	cleanup_strategy_recommendation(&recommendation);
	
	return 0;
}

/**
 * @brief Get human-readable description of clone strategy
 */
const char* get_strategy_description(enum clone_strategy strategy)
{
	switch (strategy) {
		case CLONE_STRATEGY_FULL:
			return "full (complete history and all objects)";
		case CLONE_STRATEGY_SHALLOW:
			return "shallow (limited history depth)";
		case CLONE_STRATEGY_TREELESS:
			return "treeless (on-demand tree objects)";
		case CLONE_STRATEGY_BLOBLESS:
			return "blobless (on-demand blob objects)";
		default:
			return "unknown";
	}
}

/**
 * @brief Estimate download time for different strategies
 */
int estimate_download_time(const struct repo_analysis *analysis,
	                      enum clone_strategy strategy,
	                      int bandwidth_mbps)
{
	if (!analysis || bandwidth_mbps <= 0) {
		return -1;
	}
	
	uint64_t estimated_bytes = analysis->estimated_size;
	
	/* Adjust size based on strategy */
	switch (strategy) {
		case CLONE_STRATEGY_SHALLOW:
			/* Estimate shallow clone is 10-20% of full size */
			estimated_bytes = estimated_bytes / 5;
			break;
		case CLONE_STRATEGY_TREELESS:
			/* Estimate treeless is 30-50% of full size */
			estimated_bytes = (estimated_bytes * 2) / 5;
			break;
		case CLONE_STRATEGY_BLOBLESS:
			/* Estimate blobless is 5-15% of full size */
			estimated_bytes = estimated_bytes / 8;
			break;
		case CLONE_STRATEGY_FULL:
		default:
			/* Full size */
			break;
	}
	
	/* Convert to seconds */
	double mbps_to_bytes_per_sec = (bandwidth_mbps * 1024 * 1024) / 8.0;
	double estimated_seconds = estimated_bytes / mbps_to_bytes_per_sec;
	
	return (int)ceil(estimated_seconds);
}

/**
 * @brief Check if repository supports partial clone
 */
int supports_partial_clone(const char *url)
{
	if (!url) {
		return 0;
	}
	
	/* GitHub supports partial clone */
	if (strstr(url, "github.com")) {
		return 1;
	}
	
	/* Add other known providers that support partial clone */
	if (strstr(url, "gitlab.com")) {
		return 1;
	}
	
	/* For unknown providers, assume they don't support it */
	return 0;
}

/**
 * @brief Clean up repository analysis structure
 */
void cleanup_repo_analysis(struct repo_analysis *analysis)
{
	if (!analysis) {
		return;
	}
	
	free(analysis->primary_language);
	memset(analysis, 0, sizeof(*analysis));
}

/**
 * @brief Clean up strategy recommendation structure
 */
void cleanup_strategy_recommendation(struct strategy_recommendation *recommendation)
{
	if (!recommendation) {
		return;
	}
	
	free(recommendation->reasoning);
	memset(recommendation, 0, sizeof(*recommendation));
}

/**
 * @brief Learn from user's strategy choices (placeholder for future ML)
 */
int learn_from_strategy_choice(const struct repo_info *repo,
	                          const struct repo_analysis *analysis,
	                          int success)
{
	/* For now, this is a placeholder for future machine learning integration */
	(void)repo;
	(void)analysis;
	(void)success;
	
	/* TODO: Implement learning algorithm to improve recommendations */
	return 0;
}