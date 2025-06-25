#ifndef SUBMODULE_H
#define SUBMODULE_H

/**
 * @file submodule.h
 * @brief Git submodule support for git-cache
 * 
 * Provides structures and functions for parsing .gitmodules files and
 * managing submodule caching.
 */

#include <stddef.h>

/* Forward declarations */
struct repo_info;
struct cache_config;

/**
 * @brief Information about a single git submodule
 */
struct submodule_info {
	char name[256];    /**< Submodule name from .gitmodules */
	char path[1024];   /**< Relative path in repository */
	char url[2048];    /**< Remote URL for submodule */
	char branch[256];  /**< Optional branch specification */
};

/**
 * @brief List of submodules in a repository
 */
struct submodule_list {
	struct submodule_info *submodules;  /**< Array of submodules */
	size_t count;                       /**< Number of submodules */
	size_t capacity;                    /**< Allocated capacity */
};

/**
 * @brief Parse .gitmodules file to extract submodule information
 * @param gitmodules_path Path to .gitmodules file
 * @param list Output list of submodules (must be freed with free_submodule_list)
 * @return 0 on success, -1 on error
 */
int parse_gitmodules(const char *gitmodules_path, struct submodule_list *list);

/**
 * @brief Free memory allocated for submodule list
 * @param list Submodule list to free
 */
void free_submodule_list(struct submodule_list *list);

/**
 * @brief Process all submodules in a repository
 * @param repo Repository information
 * @param config Cache configuration
 * @param recursive Whether to process submodules recursively
 * @return 0 on success, -1 on error
 */
int process_submodules(struct repo_info *repo, struct cache_config *config, int recursive);

/**
 * @brief Create cache entry for a single submodule
 * @param parent_repo Parent repository information
 * @param sub Submodule information
 * @param config Cache configuration
 * @return 0 on success, -1 on error
 */
int cache_submodule(struct repo_info *parent_repo, struct submodule_info *sub, 
                    struct cache_config *config);

/**
 * @brief Initialize submodule in checkout with reference to cache
 * @param parent_repo Parent repository information
 * @param sub Submodule information
 * @param config Cache configuration
 * @return 0 on success, -1 on error
 */
int init_submodule_checkout(struct repo_info *parent_repo, struct submodule_info *sub,
                           struct cache_config *config);

#endif /* SUBMODULE_H */