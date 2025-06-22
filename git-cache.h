#ifndef GIT_CACHE_H
#define GIT_CACHE_H

/**
 * @file git-cache.h
 * @brief Main header for git-cache - Git repository caching tool
 * 
 * Git-cache is a high-performance caching system for Git repositories that
 * provides instant access through reference-based clones and manages shared
 * repository storage efficiently.
 */

#include <stddef.h>

/** @brief Version string for git-cache */
#define VERSION "1.0.0"
/** @brief Program name */
#define PROGRAM_NAME "git-cache"

/** @brief Default cache directory relative to home */
#define CACHE_BASE_DIR ".cache/git"
/** @brief Default checkout directory */
#define CHECKOUT_BASE_DIR "github"
/** @brief Default modifiable checkout directory */
#define MODIFIABLE_BASE_DIR "github/mithro"

/**
 * @brief Repository type enumeration
 * 
 * Identifies the type of repository hosting service
 */
enum repo_type {
	REPO_TYPE_GITHUB,  /**< GitHub repository */
	REPO_TYPE_UNKNOWN  /**< Unknown or unsupported repository type */
};

/**
 * @brief Clone strategy enumeration
 * 
 * Defines different Git clone strategies for optimizing clone operations
 */
enum clone_strategy {
	CLONE_STRATEGY_FULL,     /**< Full clone with complete history */
	CLONE_STRATEGY_SHALLOW,  /**< Shallow clone with limited depth */
	CLONE_STRATEGY_TREELESS, /**< Treeless clone without trees */
	CLONE_STRATEGY_BLOBLESS  /**< Blobless clone without blobs */
};

/**
 * @brief Cache operation modes
 * 
 * Defines the available operations for git-cache
 */
enum cache_operation {
	CACHE_OP_CLONE,  /**< Clone repository with caching */
	CACHE_OP_STATUS, /**< Show cache status */
	CACHE_OP_CLEAN,  /**< Clean cache */
	CACHE_OP_SYNC,   /**< Synchronize cache with remotes */
	CACHE_OP_LIST    /**< List cached repositories */
};

/**
 * @brief Repository information structure
 * 
 * Contains all metadata and paths for a cached repository including
 * original URL, fork information, and local paths.
 */
struct repo_info {
	char *original_url;    /**< Original repository URL */
	char *fork_url;        /**< URL of the forked repository */
	char *owner;           /**< Repository owner name */
	char *name;            /**< Repository name */
	char *cache_path;      /**< Path to cached bare repository */
	char *checkout_path;   /**< Path to read-only checkout */
	char *modifiable_path; /**< Path to modifiable checkout */
	enum repo_type type;   /**< Repository hosting type */
	enum clone_strategy strategy; /**< Clone strategy used */
	int is_fork_needed;    /**< Whether forking is required */
	char *fork_organization; /**< Organization for fork (optional) */
};

/**
 * @brief Cache configuration structure
 * 
 * Global configuration settings for git-cache behavior and defaults.
 */
struct cache_config {
	char *cache_root;      /**< Root directory for cache storage */
	char *checkout_root;   /**< Root directory for checkouts */
	char *github_token;    /**< GitHub personal access token */
	enum clone_strategy default_strategy; /**< Default clone strategy */
	int verbose;           /**< Enable verbose output */
	int force;             /**< Force operations */
	int recursive_submodules; /**< Handle submodules recursively */
};

/**
 * @brief Command line options structure
 * 
 * Parsed command line arguments and options for git-cache operations.
 */
struct cache_options {
	enum cache_operation operation; /**< Operation to perform */
	char *url;             /**< Repository URL */
	char *target_path;     /**< Target path for checkout */
	enum clone_strategy strategy; /**< Clone strategy override */
	int depth;             /**< Shallow clone depth */
	int verbose;           /**< Enable verbose output */
	int force;             /**< Force operation */
	int help;              /**< Show help */
	int version;           /**< Show version */
	int recursive_submodules; /**< Handle submodules */
	char *organization;    /**< Organization for fork */
	int make_private;      /**< Make forked repository private */
};

/**
 * @defgroup public_interface Public Interface
 * @brief Public functions for git-cache
 * @{
 */

/**
 * @brief Print usage information for git-cache
 * @param program_name Name of the program (argv[0])
 */
void print_usage(const char *program_name);

/** @} */

/**
 * @defgroup error_codes Error Codes
 * @brief Return codes for git-cache operations
 * @{
 */

/** @brief Operation completed successfully */
#define CACHE_SUCCESS          0
/** @brief Invalid command line arguments */
#define CACHE_ERROR_ARGS       -1
/** @brief Configuration error */
#define CACHE_ERROR_CONFIG     -2
/** @brief Network operation failed */
#define CACHE_ERROR_NETWORK    -3
/** @brief Filesystem operation failed */
#define CACHE_ERROR_FILESYSTEM -4
/** @brief Git operation failed */
#define CACHE_ERROR_GIT        -5
/** @brief GitHub API operation failed */
#define CACHE_ERROR_GITHUB     -6
/** @brief Memory allocation failed */
#define CACHE_ERROR_MEMORY     -7

/** @} */


#endif /* GIT_CACHE_H */