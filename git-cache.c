#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <libgen.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <limits.h>

#include "git-cache.h"
#include "github_api.h"
#include "submodule.h"
#include "cache_recovery.h"
#include "cache_metadata.h"
#include "checkout_repair.h"
#include "strategy_detection.h"
#include "config_file.h"
#include "remote_sync.h"
#include "fork_config.h"
#include "shell_completion.h"

/* Lock file settings */
#define LOCK_SUFFIX ".lock"
#define LOCK_TIMEOUT 300  /* 5 minutes timeout for stale locks */
#define LOCK_WAIT_INTERVAL 100000  /* 100ms between lock attempts */
#define LOCK_MAX_ATTEMPTS 600  /* Max 60 seconds of waiting */

/* Macro for returning with lock cleanup */
#define RETURN_WITH_LOCK_CLEANUP(lock_path, retval) do { \
	release_lock(lock_path); \
	return (retval); \
} while(0)

/* Print usage information */
void print_usage(const char *program_name)
{
	printf("usage: %s <command> [options] [url]\n", program_name);
	printf("\n");
	printf("Git repository caching and mirroring tool\n");
	printf("\n");
	printf("Commands:\n");
	printf("    clone <url>        Clone repository with caching\n");
	printf("    status             Show cache status\n");
	printf("    clean              Clean cache\n");
	printf("    sync               Synchronize cache with remotes\n");
	printf("    list               List cached repositories\n");
	printf("    verify [url]       Verify cache integrity and repair if needed\n");
	printf("    repair             Repair outdated checkouts\n");
	printf("    config             Show or modify configuration\n");
	printf("    mirror             Manage remote mirrors\n");
	printf("    completion         Manage shell completion\n");
	printf("\n");
	printf("Options:\n");
	printf("    -h, --help         Show this help message\n");
	printf("    -v, --verbose      Enable verbose output\n");
	printf("    -V, --version      Show version information\n");
	printf("    -f, --force        Force operation\n");
	printf("    --strategy <type>  Clone strategy (full, shallow, treeless, blobless, auto)\n");
	printf("    --depth <n>        Depth for shallow clones (default: 1)\n");
	printf("    --org <name>       Organization for forks (default: auto-detect)\n");
	printf("    --private          Make forked repositories private\n");
	printf("    --recursive        Handle submodules recursively\n");
	printf("\n");
	printf("Examples:\n");
	printf("    %s clone https://github.com/user/repo.git\n", program_name);
	printf("    %s clone --strategy treeless git@github.com:user/repo.git\n", program_name);
	printf("    %s clone --org mithro-mirrors --private https://github.com/user/repo.git\n", program_name);
	printf("    %s status\n", program_name);
	printf("    %s clean\n", program_name);
}

/* Print version information */
static void print_version(void)
{
	printf("%s version %s\n", PROGRAM_NAME, VERSION);
	printf("Git repository caching and mirroring tool\n");
}

/* Parse clone strategy string */
static enum clone_strategy parse_strategy(const char *strategy_str)
{
	if (!strategy_str) {
	    return CLONE_STRATEGY_FULL;
	}
	
	if (strcmp(strategy_str, "full") == 0) {
	    return CLONE_STRATEGY_FULL;
	} else if (strcmp(strategy_str, "shallow") == 0) {
	    return CLONE_STRATEGY_SHALLOW;
	} else if (strcmp(strategy_str, "treeless") == 0) {
	    return CLONE_STRATEGY_TREELESS;
	} else if (strcmp(strategy_str, "blobless") == 0) {
	    return CLONE_STRATEGY_BLOBLESS;
	} else if (strcmp(strategy_str, "auto") == 0) {
	    return CLONE_STRATEGY_AUTO;
	}
	
	return CLONE_STRATEGY_FULL;
}

/* Parse command line arguments */
static int parse_arguments(int argc, char *argv[], struct cache_options *options)
{
	if (!options) {
	    return CACHE_ERROR_ARGS;
	}
	
	/* Initialize options with defaults */
	memset(options, 0, sizeof(struct cache_options));
	options->operation = CACHE_OP_CLONE;
	options->strategy = CLONE_STRATEGY_FULL;
	options->depth = 1;
	options->make_private = 0;
	options->recursive_submodules = 0;
	
	if (argc < 2) {
	    return CACHE_ERROR_ARGS;
	}
	
	int i = 1;
	
	/* Parse command */
	if (strcmp(argv[i], "clone") == 0) {
	    options->operation = CACHE_OP_CLONE;
	    i++;
	} else if (strcmp(argv[i], "status") == 0) {
	    options->operation = CACHE_OP_STATUS;
	    i++;
	} else if (strcmp(argv[i], "clean") == 0) {
	    options->operation = CACHE_OP_CLEAN;
	    i++;
	} else if (strcmp(argv[i], "sync") == 0) {
	    options->operation = CACHE_OP_SYNC;
	    i++;
	} else if (strcmp(argv[i], "list") == 0) {
	    options->operation = CACHE_OP_LIST;
	    i++;
	} else if (strcmp(argv[i], "verify") == 0) {
	    options->operation = CACHE_OP_VERIFY;
	    i++;
	    /* Optional URL argument for verify */
	    if (i < argc && argv[i][0] != '-') {
	        options->url = argv[i];
	        i++;
	    }
	} else if (strcmp(argv[i], "repair") == 0) {
	    options->operation = CACHE_OP_REPAIR;
	    i++;
	} else if (strcmp(argv[i], "config") == 0) {
	    options->operation = CACHE_OP_CONFIG;
	    i++;
	} else if (strcmp(argv[i], "mirror") == 0) {
	    options->operation = CACHE_OP_MIRROR;
	    i++;
	} else if (strcmp(argv[i], "completion") == 0) {
	    options->operation = CACHE_OP_COMPLETION;
	    i++;
	} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
	    options->help = 1;
	    return CACHE_SUCCESS;
	} else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
	    options->version = 1;
	    return CACHE_SUCCESS;
	} else {
	    /* If first argument doesn't match a command, assume it's a URL for clone */
	    options->operation = CACHE_OP_CLONE;
	    /* Don't increment i, let URL parsing handle it */
	}
	
	/* Parse options */
	while (i < argc) {
	    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
	        options->help = 1;
	        return CACHE_SUCCESS;
	    } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
	        options->verbose = 1;
	    } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
	        options->version = 1;
	        return CACHE_SUCCESS;
	    } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
	        options->force = 1;
	    } else if (strcmp(argv[i], "--private") == 0) {
	        options->make_private = 1;
	    } else if (strcmp(argv[i], "--recursive") == 0) {
	        options->recursive_submodules = 1;
	    } else if (strcmp(argv[i], "--strategy") == 0) {
	        if (i + 1 >= argc) {
	            fprintf(stderr, "error: --strategy requires an argument\n");
	            return CACHE_ERROR_ARGS;
	        }
	        options->strategy = parse_strategy(argv[i + 1]);
	        i++; /* Skip the strategy argument */
	    } else if (strcmp(argv[i], "--depth") == 0) {
	        if (i + 1 >= argc) {
	            fprintf(stderr, "error: --depth requires an argument\n");
	            return CACHE_ERROR_ARGS;
	        }
	        options->depth = atoi(argv[i + 1]);
	        if (options->depth <= 0) {
	            fprintf(stderr, "error: depth must be positive\n");
	            return CACHE_ERROR_ARGS;
	        }
	        i++; /* Skip the depth argument */
	    } else if (strcmp(argv[i], "--org") == 0) {
	        if (i + 1 >= argc) {
	            fprintf(stderr, "error: --org requires an argument\n");
	            return CACHE_ERROR_ARGS;
	        }
	        options->organization = argv[i + 1];
	        i++; /* Skip the organization argument */
	    } else if (argv[i][0] == '-') {
	        fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
	        return CACHE_ERROR_ARGS;
	    } else {
	        /* Non-option argument - should be URL or target path */
	        if (!options->url) {
	            options->url = argv[i];
	        } else if (!options->target_path) {
	            options->target_path = argv[i];
	        } else {
	            fprintf(stderr, "error: too many arguments\n");
	            return CACHE_ERROR_ARGS;
	        }
	    }
	    i++;
	}
	
	/* Validate arguments based on operation */
	if (options->operation == CACHE_OP_CLONE && !options->url) {
	    fprintf(stderr, "error: clone operation requires a URL\n");
	    return CACHE_ERROR_ARGS;
	}
	
	return CACHE_SUCCESS;
}

/* Get error string for error code */
static const char* cache_get_error_string(int error_code)
{
	switch (error_code) {
	    case CACHE_SUCCESS:
	        return "Success";
	    case CACHE_ERROR_ARGS:
	        return "Invalid arguments";
	    case CACHE_ERROR_CONFIG:
	        return "Configuration error";
	    case CACHE_ERROR_NETWORK:
	        return "Network error";
	    case CACHE_ERROR_FILESYSTEM:
	        return "Filesystem error";
	    case CACHE_ERROR_GIT:
	        return "Git operation error";
	    case CACHE_ERROR_GITHUB:
	        return "GitHub API error";
	    case CACHE_ERROR_MEMORY:
	        return "Memory allocation error";
	    default:
	        return "Unknown error";
	}
}

/* Check if running in a git repository */
static int is_git_repository(void)
{
	return system("git rev-parse --git-dir >/dev/null 2>&1") == 0;
}

/* Utility functions */

/* Get current working directory */
static char* get_current_directory(void)
{
	char *cwd = getcwd(NULL, 0);
	return cwd; /* getcwd allocates memory for us */
}

/* Get home directory */
static char* get_home_directory(void)
{
	const char *home = getenv("HOME");
	if (!home) {
	    return NULL;
	}
	
	char *result = malloc(strlen(home) + 1);
	if (result) {
	    strcpy(result, home);
	}
	return result;
}

/* Resolve path with environment variable expansion */
static char* resolve_path(const char *path)
{
	if (!path) {
	    return NULL;
	}
	
	/* If path starts with ~, expand to home directory */
	if (path[0] == '~') {
	    char *home = get_home_directory();
	    if (!home) {
	        return NULL;
	    }
	    
	    size_t home_len = strlen(home);
	    size_t path_len = strlen(path);
	    char *result = malloc(home_len + path_len);
	    if (!result) {
	        free(home);
	        return NULL;
	    }
	    
	    strcpy(result, home);
	    strcat(result, path + 1); /* Skip the ~ */
	    free(home);
	    return result;
	}
	
	/* Otherwise, just copy the path */
	char *result = malloc(strlen(path) + 1);
	if (result) {
	    strcpy(result, path);
	}
	return result;
}

/* Check if directory exists and is accessible */
static int directory_exists(const char *path)
{
	struct stat st;
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

/* Ensure directory exists, creating if necessary */
static int ensure_directory_exists(const char *path)
{
	if (!path) {
	    return CACHE_ERROR_ARGS;
	}
	
	if (directory_exists(path)) {
	    return CACHE_SUCCESS;
	}
	
	/* Create directory recursively */
	char *path_copy = malloc(strlen(path) + 1);
	if (!path_copy) {
	    return CACHE_ERROR_MEMORY;
	}
	strcpy(path_copy, path);
	
	const char *dir = dirname(path_copy);
	if (strcmp(dir, "/") != 0 && strcmp(dir, ".") != 0) {
	    int ret = ensure_directory_exists(dir);
	    if (ret != CACHE_SUCCESS) {
	        free(path_copy);
	        return ret;
	    }
	}
	
	free(path_copy);
	
	if (mkdir(path, 0755) != 0 && errno != EEXIST) {
	    return CACHE_ERROR_FILESYSTEM;
	}
	
	return CACHE_SUCCESS;
}

/* Configuration management */

/* Create cache configuration with defaults */
static struct cache_config* cache_config_create(void)
{
	struct cache_config *config = malloc(sizeof(struct cache_config));
	if (!config) {
	    return NULL;
	}
	
	memset(config, 0, sizeof(struct cache_config));
	
	/* Set cache root from GIT_CACHE environment variable or default to ~/.cache/git */
	const char *git_cache = getenv("GIT_CACHE");
	if (git_cache) {
	    config->cache_root = resolve_path(git_cache);
	} else {
	    /* Default to ~/.cache/git */
	    char *home = get_home_directory();
	    if (home) {
	        size_t cache_len = strlen(home) + 1 + strlen(CACHE_BASE_DIR) + 1;
	        config->cache_root = malloc(cache_len);
	        if (config->cache_root) {
	            snprintf(config->cache_root, cache_len, "%s/%s", home, CACHE_BASE_DIR);
	        }
	        free(home);
	    }
	}
	
	/* Set checkout root relative to current directory by default */
	char *cwd = get_current_directory();
	if (cwd) {
	    size_t checkout_len = strlen(cwd) + 1 + strlen(CHECKOUT_BASE_DIR) + 1;
	    config->checkout_root = malloc(checkout_len);
	    if (config->checkout_root) {
	        snprintf(config->checkout_root, checkout_len, "%s/%s", cwd, CHECKOUT_BASE_DIR);
	    }
	    free(cwd);
	}
	
	/* Fallback to home directory if no cache root could be determined */
	if (!config->cache_root) {
	    char *home = get_home_directory();
	    if (home) {
	        size_t cache_len = strlen(home) + 1 + strlen(CACHE_BASE_DIR) + 1;
	        config->cache_root = malloc(cache_len);
	        if (config->cache_root) {
	            snprintf(config->cache_root, cache_len, "%s/%s", home, CACHE_BASE_DIR);
	        }
	        free(home);
	    }
	}
	
	/* Set default strategy */
	config->default_strategy = CLONE_STRATEGY_TREELESS;
	config->verbose = 0;
	config->force = 0;
	config->recursive_submodules = 1;
	
	/* Allocate and initialize fork configuration */
	config->fork_config = malloc(sizeof(struct fork_config));
	if (config->fork_config) {
		get_default_fork_config((struct fork_config *)config->fork_config);
	}
	
	/* Get GitHub token from environment */
	const char *token = getenv("GITHUB_TOKEN");
	if (token) {
	    config->github_token = malloc(strlen(token) + 1);
	    if (config->github_token) {
	        strcpy(config->github_token, token);
	    }
	}
	
	return config;
}

/* Destroy cache configuration */
static void cache_config_destroy(struct cache_config *config)
{
	if (!config) {
	    return;
	}
	
	free(config->cache_root);
	free(config->checkout_root);
	free(config->github_token);
	
	/* Clean up fork configuration */
	if (config->fork_config) {
		cleanup_fork_config((struct fork_config *)config->fork_config);
		free(config->fork_config);
	}
	
	free(config);
}

/* Load configuration from environment and config files */
static int cache_config_load(struct cache_config *config)
{
	if (!config) {
	    return CACHE_ERROR_ARGS;
	}
	
	/* First, load configuration from files (in order of precedence) */
	/* This will load from system config, user config, and local config */
	int ret = load_configuration(config);
	if (ret != CONFIG_SUCCESS && ret != CONFIG_ERROR_NOT_FOUND) {
	    /* Only fail if there's an actual error, not if config files don't exist */
	    fprintf(stderr, "warning: failed to load configuration files: %s\n", 
	            config_get_error_string(ret));
	    /* Continue anyway - we'll use defaults and environment variables */
	}
	
	/* Load fork configuration */
	if (config->fork_config) {
		load_fork_config((struct fork_config *)config->fork_config);
	}
	
	/* Override with environment variables if set */
	/* GIT_CACHE takes precedence over GIT_CACHE_ROOT for backward compatibility */
	const char *env_cache_root = getenv("GIT_CACHE");
	if (!env_cache_root) {
	    env_cache_root = getenv("GIT_CACHE_ROOT");
	}
	if (env_cache_root) {
	    free(config->cache_root);
	    config->cache_root = resolve_path(env_cache_root);
	    if (!config->cache_root) {
	        return CACHE_ERROR_MEMORY;
	    }
	}
	
	const char *env_checkout_root = getenv("GIT_CHECKOUT_ROOT");
	if (env_checkout_root) {
	    free(config->checkout_root);
	    config->checkout_root = resolve_path(env_checkout_root);
	    if (!config->checkout_root) {
	        return CACHE_ERROR_MEMORY;
	    }
	}
	
	const char *env_token = getenv("GITHUB_TOKEN");
	if (env_token) {
	    free(config->github_token);
	    config->github_token = malloc(strlen(env_token) + 1);
	    if (!config->github_token) {
	        return CACHE_ERROR_MEMORY;
	    }
	    strcpy(config->github_token, env_token);
	}
	
	/* Also check for environment variable that points to a specific config file */
	const char *env_config_file = getenv(CONFIG_ENV_VAR);
	if (env_config_file) {
	    ret = load_config_file(env_config_file, config);
	    if (ret != CONFIG_SUCCESS) {
	        fprintf(stderr, "warning: failed to load config from %s: %s\n",
	                env_config_file, config_get_error_string(ret));
	    }
	}
	
	return CACHE_SUCCESS;
}

/* Validate cache configuration */
static int cache_config_validate(const struct cache_config *config)
{
	if (!config) {
	    return CACHE_ERROR_ARGS;
	}
	
	if (!config->cache_root) {
	    fprintf(stderr, "error: cache root directory not set\n");
	    return CACHE_ERROR_CONFIG;
	}
	
	if (!config->checkout_root) {
	    fprintf(stderr, "error: checkout root directory not set\n");
	    return CACHE_ERROR_CONFIG;
	}
	
	/* Ensure cache and checkout directories exist */
	int ret = ensure_directory_exists(config->cache_root);
	if (ret != CACHE_SUCCESS) {
	    fprintf(stderr, "error: failed to create cache directory '%s'\n", config->cache_root);
	    return ret;
	}
	
	ret = ensure_directory_exists(config->checkout_root);
	if (ret != CACHE_SUCCESS) {
	    fprintf(stderr, "error: failed to create checkout directory '%s'\n", config->checkout_root);
	    return ret;
	}
	
	return CACHE_SUCCESS;
}

/* Repository information management */

/* Create repository information structure */
static struct repo_info* repo_info_create(void)
{
	struct repo_info *repo = malloc(sizeof(struct repo_info));
	if (!repo) {
	    return NULL;
	}
	
	memset(repo, 0, sizeof(struct repo_info));
	repo->type = REPO_TYPE_UNKNOWN;
	repo->strategy = CLONE_STRATEGY_FULL;
	repo->is_fork_needed = 0;
	
	return repo;
}

/* Destroy repository information structure */
void repo_info_destroy(struct repo_info *repo)
{
	if (!repo) {
	    return;
	}
	
	free(repo->original_url);
	free(repo->fork_url);
	free(repo->owner);
	free(repo->name);
	free(repo->cache_path);
	free(repo->checkout_path);
	free(repo->modifiable_path);
	free(repo->fork_organization);
	free(repo);
}

/* Parse repository URL and extract information */
int repo_info_parse_url(const char *url, struct repo_info *repo)
{
	if (!url || !repo) {
	    return CACHE_ERROR_ARGS;
	}
	
	/* Store original URL */
	repo->original_url = malloc(strlen(url) + 1);
	if (!repo->original_url) {
	    return CACHE_ERROR_MEMORY;
	}
	strcpy(repo->original_url, url);
	
	/* Parse GitHub URLs */
	char *owner, *name;
	int ret = github_parse_repo_url(url, &owner, &name);
	if (ret == GITHUB_SUCCESS) {
	    repo->type = REPO_TYPE_GITHUB;
	    repo->owner = owner;
	    repo->name = name;
	    repo->is_fork_needed = 0; /* Will be determined by fork configuration */
	    return CACHE_SUCCESS;
	}
	
	/* If GitHub parsing failed, it's an unknown repository type */
	repo->type = REPO_TYPE_UNKNOWN;
	fprintf(stderr, "warning: unsupported repository URL format '%s'\n", url);
	
	return CACHE_ERROR_ARGS;
}

/* Setup paths for repository based on configuration */
static int repo_info_setup_paths(struct repo_info *repo, const struct cache_config *config)
{
	if (!repo || !config) {
	    return CACHE_ERROR_ARGS;
	}
	
	if (repo->type != REPO_TYPE_GITHUB) {
	    return CACHE_ERROR_ARGS;
	}
	
	if (!repo->owner || !repo->name) {
	    return CACHE_ERROR_ARGS;
	}
	
	/* Setup cache path: ~/.cache/git/github.com/owner/repo */
	size_t cache_len = strlen(config->cache_root) + strlen("/github.com/") + 
	                   strlen(repo->owner) + 1 + strlen(repo->name) + 1;
	repo->cache_path = malloc(cache_len);
	if (!repo->cache_path) {
	    return CACHE_ERROR_MEMORY;
	}
	snprintf(repo->cache_path, cache_len, "%s/github.com/%s/%s", 
	         config->cache_root, repo->owner, repo->name);
	
	/* Setup checkout path: ~/github/owner/repo */
	size_t checkout_len = strlen(config->checkout_root) + 1 + 
	                      strlen(repo->owner) + 1 + strlen(repo->name) + 1;
	repo->checkout_path = malloc(checkout_len);
	if (!repo->checkout_path) {
	    return CACHE_ERROR_MEMORY;
	}
	snprintf(repo->checkout_path, checkout_len, "%s/%s/%s", 
	         config->checkout_root, repo->owner, repo->name);
	
	/* Setup modifiable path: ./github/mithro/owner-repo */
	size_t modifiable_len = strlen(config->checkout_root) + strlen("/mithro/") + 
	                        strlen(repo->owner) + 1 + strlen(repo->name) + 1;
	repo->modifiable_path = malloc(modifiable_len);
	if (!repo->modifiable_path) {
	    return CACHE_ERROR_MEMORY;
	}
	snprintf(repo->modifiable_path, modifiable_len, "%s/mithro/%s-%s", 
	         config->checkout_root, repo->owner, repo->name);
	
	return CACHE_SUCCESS;
}

/* Git operation helpers - forward declarations */
static int run_git_command(const char *command, const char *working_dir);
static int scan_cache_directory(const char *cache_dir, const struct cache_config *config, 
	                           const struct cache_options *options);
static int create_reference_checkout(const char *cache_path, const char *checkout_path,
	                                enum clone_strategy strategy, const struct cache_options *options,
	                                const struct cache_config *config, const char *original_url);

/* Git operation helpers */

/* Check if a file exists */
static int file_exists(const char *path)
{
	if (!path) {
	    return 0;
	}
	FILE *file = fopen(path, "r");
	if (file) {
	    fclose(file);
	    return 1;
	}
	return 0;
}

/* Check if a git repository exists at the given path */
static int is_git_repository_at(const char *path)
{
	if (!path) {
	    return 0;
	}
	
	/* Check if directory exists first */
	if (!directory_exists(path)) {
	    return 0;
	}
	
	/* For bare repositories, check for git files directly */
	size_t path_len = strlen(path);
	
	/* Check for HEAD file (exists in both bare and non-bare repos) */
	char *head_path = malloc(path_len + strlen("/HEAD") + 1);
	if (!head_path) {
	    return 0;
	}
	snprintf(head_path, path_len + strlen("/HEAD") + 1, "%s/HEAD", path);
	
	/* Check for refs directory */
	char *refs_path = malloc(path_len + strlen("/refs") + 1);
	if (!refs_path) {
	    free(head_path);
	    return 0;
	}
	snprintf(refs_path, path_len + strlen("/refs") + 1, "%s/refs", path);
	
	/* Check for objects directory */
	char *objects_path = malloc(path_len + strlen("/objects") + 1);
	if (!objects_path) {
	    free(head_path);
	    free(refs_path);
	    return 0;
	}
	snprintf(objects_path, path_len + strlen("/objects") + 1, "%s/objects", path);
	
	/* A git repository should have HEAD file and refs + objects directories */
	struct stat st;
	int head_exists = (stat(head_path, &st) == 0 && S_ISREG(st.st_mode));
	int is_git_repo = head_exists && 
	                  directory_exists(refs_path) && 
	                  directory_exists(objects_path);
	
	free(head_path);
	free(refs_path);
	free(objects_path);
	
	return is_git_repo;
}

/* Simple progress indicator for long-running operations */
static void show_progress_indicator(const char *operation, int show_spinner)
{
	static int spinner_pos = 0;
	static const char spinner[] = "|/-\\";
	
	if (!operation) {
		return;
	}
	
	if (show_spinner) {
		printf("\r%s %c", operation, spinner[spinner_pos]);
		fflush(stdout);
		spinner_pos = (spinner_pos + 1) % 4;
	} else {
		printf("\r%s... ", operation);
		fflush(stdout);
	}
}

/* Clear progress indicator line */
static void clear_progress_indicator(void)
{
	printf("\r");
	fflush(stdout);
}

/* Enhanced progress wrapper for git operations */
static int run_git_command_with_progress(const char *cmd, const char *working_dir, const char *operation)
{
	if (!cmd || !operation) {
		return -1;
	}
	
	/* Show initial progress */
	show_progress_indicator(operation, 0);
	
	/* For simple operations, just run the command */
	int result = run_git_command(cmd, working_dir);
	
	/* Clear progress indicator */
	clear_progress_indicator();
	
	return result;
}

/* Check available disk space for operations */
static int check_disk_space(const char *path, size_t required_mb)
{
	if (!path) {
	    return 0;
	}
	
	/* Use df command to check available space */
	char *df_cmd = malloc(strlen("df -m \"") + strlen(path) + 
	                     strlen("\" | tail -1 | awk '{print $4}'") + 1);
	if (!df_cmd) {
	    return 0;
	}
	
	snprintf(df_cmd, strlen("df -m \"") + strlen(path) + 
	         strlen("\" | tail -1 | awk '{print $4}'") + 1,
	         "df -m \"%s\" | tail -1 | awk '{print $4}'", path);
	
	FILE *fp = popen(df_cmd, "r");
	free(df_cmd);
	
	if (!fp) {
	    return 0;
	}
	
	char space_str[32];
	if (fgets(space_str, sizeof(space_str), fp) == NULL) {
	    pclose(fp);
	    return 0;
	}
	
	pclose(fp);
	
	/* Parse available space in MB */
	char *endptr;
	long available_mb = strtol(space_str, &endptr, 10);
	
	if (endptr == space_str || available_mb < 0) {
	    /* If we can't determine space, assume it's okay */
	    return 1;
	}
	
	return (size_t)available_mb >= required_mb;
}

/* Retry network operation with exponential backoff and progress */
static int retry_network_operation_with_progress(const char *cmd, int max_retries, const struct cache_config *config, const char *operation)
{
	if (!cmd) {
	    return -1;
	}
	
	int attempt = 0;
	int delay = 1; /* Start with 1 second delay */
	
	while (attempt < max_retries) {
	    if (config->verbose && attempt > 0) {
	        printf("Retrying network operation (attempt %d/%d)...\n", attempt + 1, max_retries);
	    } else if (operation) {
	        show_progress_indicator(operation, 0);
	    }
	    
	    int result = system(cmd);
	    int exit_code = WEXITSTATUS(result);
	    
	    /* Clear progress if shown */
	    if (operation && (!config->verbose || attempt == 0)) {
	        clear_progress_indicator();
	    }
	    
	    /* Success */
	    if (exit_code == 0) {
	        return 0;
	    }
	    
	    /* Non-recoverable errors (authentication, not found, etc.) */
	    if (exit_code == 128 || exit_code == 1) {
	        return exit_code;
	    }
	    
	    attempt++;
	    
	    /* If not the last attempt, wait before retrying */
	    if (attempt < max_retries) {
	        if (config->verbose) {
	            printf("Network operation failed with code %d, waiting %d seconds before retry...\n", 
	                   exit_code, delay);
	        }
	        sleep(delay);
	        delay *= 2; /* Exponential backoff */
	        if (delay > 16) delay = 16; /* Cap at 16 seconds */
	    }
	}
	
	return -1; /* All retries failed */
}


/* Perform deep integrity validation using git commands */
static int validate_git_repository_integrity(const char *repo_path, int is_bare)
{
	if (!repo_path) {
	    return 0;
	}
	
	/* Build command to check repository integrity */
	char *integrity_cmd = malloc(strlen("cd \"") + strlen(repo_path) + 
	                           strlen("\" && git rev-parse --git-dir >/dev/null 2>&1") + 1);
	if (!integrity_cmd) {
	    return 0;
	}
	
	snprintf(integrity_cmd, strlen("cd \"") + strlen(repo_path) + 
	         strlen("\" && git rev-parse --git-dir >/dev/null 2>&1") + 1,
	         "cd \"%s\" && git rev-parse --git-dir >/dev/null 2>&1", repo_path);
	
	int result = system(integrity_cmd);
	free(integrity_cmd);
	
	if (WEXITSTATUS(result) != 0) {
	    return 0;
	}
	
	/* For bare repositories, check if we can list refs */
	if (is_bare) {
	    char *refs_cmd = malloc(strlen("cd \"") + strlen(repo_path) + 
	                          strlen("\" && git show-ref >/dev/null 2>&1") + 1);
	    if (!refs_cmd) {
	        return 0;
	    }
	    
	    snprintf(refs_cmd, strlen("cd \"") + strlen(repo_path) + 
	             strlen("\" && git show-ref >/dev/null 2>&1") + 1,
	             "cd \"%s\" && git show-ref >/dev/null 2>&1", repo_path);
	    
	    int refs_result = system(refs_cmd);
	    free(refs_cmd);
	    
	    /* Empty repositories might have no refs, which is okay */
	    /* We just check that the command doesn't fail with corruption errors */
	    if (WEXITSTATUS(refs_result) > 1) {
	        return 0;
	    }
	} else {
	    /* For working tree, check if we can get HEAD */
	    char *head_cmd = malloc(strlen("cd \"") + strlen(repo_path) + 
	                          strlen("\" && git rev-parse HEAD >/dev/null 2>&1") + 1);
	    if (!head_cmd) {
	        return 0;
	    }
	    
	    snprintf(head_cmd, strlen("cd \"") + strlen(repo_path) + 
	             strlen("\" && git rev-parse HEAD >/dev/null 2>&1") + 1,
	             "cd \"%s\" && git rev-parse HEAD >/dev/null 2>&1", repo_path);
	    
	    int head_result = system(head_cmd);
	    free(head_cmd);
	    
	    /* Allow newly created repositories or repositories without commits */
	    if (WEXITSTATUS(head_result) > 128) {
	        return 0;
	    }
	}
	
	return 1;
}

/* Validate that a git repository is healthy and complete */
static int validate_git_repository(const char *repo_path, int is_bare)
{
	if (!repo_path) {
	    return 0;
	}
	
	/* For working tree repositories, git files are in .git subdirectory */
	char *git_dir_path;
	if (is_bare) {
	    git_dir_path = malloc(strlen(repo_path) + 1);
	    if (!git_dir_path) {
	        return 0;
	    }
	    strcpy(git_dir_path, repo_path);
	} else {
	    git_dir_path = malloc(strlen(repo_path) + strlen("/.git") + 1);
	    if (!git_dir_path) {
	        return 0;
	    }
	    snprintf(git_dir_path, strlen(repo_path) + strlen("/.git") + 1, "%s/.git", repo_path);
	}
	
	/* Check if the git directory exists */
	if (!directory_exists(git_dir_path)) {
	    free(git_dir_path);
	    return 0;
	}
	
	/* Check for essential git files/directories */
	char *objects_path = malloc(strlen(git_dir_path) + strlen("/objects") + 1);
	if (!objects_path) {
	    free(git_dir_path);
	    return 0;
	}
	snprintf(objects_path, strlen(git_dir_path) + strlen("/objects") + 1, "%s/objects", git_dir_path);
	
	char *refs_path = malloc(strlen(git_dir_path) + strlen("/refs") + 1);
	if (!refs_path) {
	    free(git_dir_path);
	    free(objects_path);
	    return 0;
	}
	snprintf(refs_path, strlen(git_dir_path) + strlen("/refs") + 1, "%s/refs", git_dir_path);
	
	char *head_path = malloc(strlen(git_dir_path) + strlen("/HEAD") + 1);
	if (!head_path) {
	    free(git_dir_path);
	    free(objects_path);
	    free(refs_path);
	    return 0;
	}
	snprintf(head_path, strlen(git_dir_path) + strlen("/HEAD") + 1, "%s/HEAD", git_dir_path);
	
	int basic_valid = directory_exists(objects_path) && 
	                 directory_exists(refs_path) && 
	                 file_exists(head_path);
	
	free(git_dir_path);
	free(objects_path);
	free(refs_path);
	free(head_path);
	
	if (!basic_valid) {
	    return 0;
	}
	
	/* Perform deeper validation using git commands */
	return validate_git_repository_integrity(repo_path, is_bare);
}

/* Safely remove directory with validation */
static int safe_remove_directory(const char *path, const struct cache_config *config)
{
	if (!path || strlen(path) < 3) {
	    return CACHE_ERROR_ARGS;
	}
	
	/* Safety check: ensure we're not removing system directories */
	if (strcmp(path, "/") == 0 || strcmp(path, "/home") == 0 || 
	    strcmp(path, "/usr") == 0 || strcmp(path, "/var") == 0) {
	    if (config->verbose) {
	        printf("Error: Refusing to remove system directory: %s\n", path);
	    }
	    return CACHE_ERROR_ARGS;
	}
	
	if (config->verbose) {
	    printf("Safely removing directory: %s\n", path);
	}
	
	char *rm_cmd = malloc(strlen("rm -rf \"") + strlen(path) + strlen("\"") + 1);
	if (!rm_cmd) {
	    return CACHE_ERROR_MEMORY;
	}
	snprintf(rm_cmd, strlen("rm -rf \"") + strlen(path) + strlen("\"") + 1,
	         "rm -rf \"%s\"", path);
	
	int result = system(rm_cmd);
	free(rm_cmd);
	
	if (WEXITSTATUS(result) != 0) {
	    if (config->verbose) {
	        printf("Warning: Failed to remove directory %s\n", path);
	    }
	    return CACHE_ERROR_FILESYSTEM;
	}
	
	return CACHE_SUCCESS;
}

/* Create a backup of an existing repository */
static int backup_repository(const char *repo_path, char **backup_path, const struct cache_config *config)
{
	if (!repo_path || !backup_path) {
	    return CACHE_ERROR_ARGS;
	}
	
	*backup_path = malloc(strlen(repo_path) + strlen(".backup.") + 16);
	if (!*backup_path) {
	    return CACHE_ERROR_MEMORY;
	}
	
	/* Create backup with timestamp */
	time_t now = time(NULL);
	snprintf(*backup_path, strlen(repo_path) + strlen(".backup.") + 16,
	         "%s.backup.%ld", repo_path, (long)now);
	
	if (config->verbose) {
	    printf("Creating backup: %s -> %s\n", repo_path, *backup_path);
	}
	
	char *mv_cmd = malloc(strlen("mv \"") + strlen(repo_path) + strlen("\" \"") + 
	                     strlen(*backup_path) + strlen("\"") + 1);
	if (!mv_cmd) {
	    free(*backup_path);
	    *backup_path = NULL;
	    return CACHE_ERROR_MEMORY;
	}
	
	snprintf(mv_cmd, strlen("mv \"") + strlen(repo_path) + strlen("\" \"") + 
	         strlen(*backup_path) + strlen("\"") + 1,
	         "mv \"%s\" \"%s\"", repo_path, *backup_path);
	
	int result = system(mv_cmd);
	free(mv_cmd);
	
	if (WEXITSTATUS(result) != 0) {
	    free(*backup_path);
	    *backup_path = NULL;
	    return CACHE_ERROR_FILESYSTEM;
	}
	
	return CACHE_SUCCESS;
}

/* Restore a repository from backup */
static int restore_from_backup(const char *backup_path, const char *repo_path, const struct cache_config *config)
{
	if (!backup_path || !repo_path) {
	    return CACHE_ERROR_ARGS;
	}
	
	if (config->verbose) {
	    printf("Restoring from backup: %s -> %s\n", backup_path, repo_path);
	}
	
	char *mv_cmd = malloc(strlen("mv \"") + strlen(backup_path) + strlen("\" \"") + 
	                     strlen(repo_path) + strlen("\"") + 1);
	if (!mv_cmd) {
	    return CACHE_ERROR_MEMORY;
	}
	
	snprintf(mv_cmd, strlen("mv \"") + strlen(backup_path) + strlen("\" \"") + 
	         strlen(repo_path) + strlen("\"") + 1,
	         "mv \"%s\" \"%s\"", backup_path, repo_path);
	
	int result = system(mv_cmd);
	free(mv_cmd);
	
	return (WEXITSTATUS(result) == 0) ? CACHE_SUCCESS : CACHE_ERROR_FILESYSTEM;
}

/* Execute git command and return exit code */
static int run_git_command(const char *command, const char *working_dir)
{
	if (!command) {
	    return -1;
	}
	
	char *full_command;
	if (working_dir) {
	    size_t cmd_len = strlen("cd \"") + strlen(working_dir) + strlen("\" && ") + strlen(command) + 1;
	    full_command = malloc(cmd_len);
	    if (!full_command) {
	        return -1;
	    }
	    snprintf(full_command, cmd_len, "cd \"%s\" && %s", working_dir, command);
	} else {
	    full_command = malloc(strlen(command) + 1);
	    if (!full_command) {
	        return -1;
	    }
	    strcpy(full_command, command);
	}
	
	int result = system(full_command);
	free(full_command);
	return WEXITSTATUS(result);
}

/* Lock file management functions */

/* Get lock file path for a given resource */
static char* get_lock_path(const char *resource_path)
{
	if (!resource_path) {
	    return NULL;
	}
	
	size_t len = strlen(resource_path) + strlen(LOCK_SUFFIX) + 1;
	char *lock_path = malloc(len);
	if (!lock_path) {
	    return NULL;
	}
	
	snprintf(lock_path, len, "%s%s", resource_path, LOCK_SUFFIX);
	return lock_path;
}

/* Check if a lock file is stale (older than timeout) */
static int is_lock_stale(const char *lock_path)
{
	struct stat st;
	if (stat(lock_path, &st) != 0) {
	    return 1;  /* Lock doesn't exist or can't be accessed */
	}
	
	time_t now = time(NULL);
	time_t age = now - st.st_mtime;
	
	return age > LOCK_TIMEOUT;
}

/* Write PID to lock file */
static int write_lock_pid(int fd)
{
	char pid_str[32];
	snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
	
	if (ftruncate(fd, 0) < 0) {
	    return -1;
	}
	
	if (lseek(fd, 0, SEEK_SET) < 0) {
	    return -1;
	}
	
	ssize_t written = write(fd, pid_str, strlen(pid_str));
	return (written == (ssize_t)strlen(pid_str)) ? 0 : -1;
}

/* Read PID from lock file */
static pid_t read_lock_pid(const char *lock_path)
{
	FILE *f = fopen(lock_path, "r");
	if (!f) {
	    return -1;
	}
	
	pid_t pid = -1;
	if (fscanf(f, "%d", &pid) != 1) {
		pid = -1;
	}
	fclose(f);
	
	return pid;
}

/* Check if a process is still running */
static int is_process_running(pid_t pid)
{
	if (pid <= 0) {
	    return 0;
	}
	
	/* Check if process exists by sending signal 0 */
	return kill(pid, 0) == 0;
}

/* Acquire a lock on a resource with timeout and stale lock handling */
static int acquire_lock(const char *resource_path, const struct cache_config *config)
{
	char *lock_path = get_lock_path(resource_path);
	if (!lock_path) {
	    return CACHE_ERROR_MEMORY;
	}
	
	int attempts = 0;
	
	while (attempts < LOCK_MAX_ATTEMPTS) {
	    /* Try to create lock file exclusively */
	    int lock_fd = open(lock_path, O_CREAT | O_EXCL | O_WRONLY, 0644);
	    
	    if (lock_fd >= 0) {
	        /* Successfully created lock file */
	        if (write_lock_pid(lock_fd) == 0) {
	            close(lock_fd);
	            free(lock_path);
	            return CACHE_SUCCESS;
	        }
	        /* Failed to write PID, cleanup */
	        close(lock_fd);
	        unlink(lock_path);
	        free(lock_path);
	        return CACHE_ERROR_FILESYSTEM;
	    }
	    
	    /* Lock file exists, check if it's stale */
	    if (errno == EEXIST) {
	        if (is_lock_stale(lock_path)) {
	            /* Check if the process holding the lock is still running */
	            pid_t holder_pid = read_lock_pid(lock_path);
	            if (holder_pid <= 0 || !is_process_running(holder_pid)) {
	                if (config->verbose) {
	                    printf("Removing stale lock file: %s (PID %d)\n", lock_path, holder_pid);
	                }
	                unlink(lock_path);
	                continue;  /* Try again */
	            }
	        }
	        
	        /* Lock is held by another process */
	        if (attempts == 0 && config->verbose) {
	            pid_t holder_pid = read_lock_pid(lock_path);
	            printf("Waiting for lock held by PID %d...\n", holder_pid);
	        }
	        
	        usleep(LOCK_WAIT_INTERVAL);
	        attempts++;
	    } else {
	        /* Other error */
	        if (config->verbose) {
	            printf("Failed to create lock file: %s\n", strerror(errno));
	        }
	        free(lock_path);
	        return CACHE_ERROR_FILESYSTEM;
	    }
	}
	
	/* Timeout waiting for lock */
	if (config->verbose) {
	    printf("Timeout waiting for lock on: %s\n", resource_path);
	}
	free(lock_path);
	return CACHE_ERROR_FILESYSTEM;
}

/* Release a lock on a resource */
static int release_lock(const char *resource_path)
{
	char *lock_path = get_lock_path(resource_path);
	if (!lock_path) {
	    return CACHE_ERROR_MEMORY;
	}
	
	/* Only remove if we own it */
	pid_t lock_pid = read_lock_pid(lock_path);
	if (lock_pid == getpid()) {
	    unlink(lock_path);
	}
	
	free(lock_path);
	return CACHE_SUCCESS;
}

/* Cleanup function to release locks on exit - for future signal handler use */
/* Currently unused but kept for future enhancement */

/* Create full bare repository in cache location with robust error handling */
static int create_cache_repository(const struct repo_info *repo, const struct cache_config *config)
{
	if (!repo || !config) {
	    return CACHE_ERROR_ARGS;
	}
	
	if (config->verbose) {
	    printf("Creating cache repository at: %s\n", repo->cache_path);
	}
	
	/* Acquire lock for cache repository operations */
	int lock_ret = acquire_lock(repo->cache_path, config);
	if (lock_ret != CACHE_SUCCESS) {
	    fprintf(stderr, "error: failed to acquire lock for cache repository\n");
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, lock_ret);
	}
	
	char *backup_path = NULL;
	
	/* Check if cache already exists */
	if (directory_exists(repo->cache_path)) {
	    if (is_git_repository_at(repo->cache_path)) {
	        /* Validate existing repository */
	        if (validate_git_repository(repo->cache_path, 1)) {
	            if (config->verbose) {
	                printf("Valid cache repository found, updating...\n");
	            }
	            
	            /* Update existing repository */
	            char *fetch_cmd = malloc(strlen("git fetch origin '+refs/heads/*:refs/heads/*' --prune") + 1);
	            if (!fetch_cmd) {
	                RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_MEMORY);
	            }
	            strcpy(fetch_cmd, "git fetch origin '+refs/heads/*:refs/heads/*' --prune");
	            
	            int result = run_git_command_with_progress(fetch_cmd, repo->cache_path, "Updating cache repository");
	            free(fetch_cmd);
	            
	            if (result != 0) {
	                if (config->verbose) {
	                    printf("Warning: git fetch failed with exit code %d\n", result);
	                }
	                /* Don't fail if fetch fails - the cache might still be usable */
	                printf("Note: Using existing cache (fetch failed but cache is still valid)\n");
	            } else {
	                /* Update metadata sync time after successful fetch */
	                int metadata_ret = cache_metadata_update_sync(repo->cache_path);
	                if (metadata_ret != METADATA_SUCCESS) {
	                    if (config->verbose) {
	                        printf("Warning: Failed to update cache metadata sync time (error %d)\n", metadata_ret);
	                    }
	                } else if (config->verbose) {
	                    printf("Cache metadata sync time updated\n");
	                }
	            }
	            
	            RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_SUCCESS);
	        } else {
	            /* Repository exists but is corrupted - back it up and recreate */
	            if (config->verbose) {
	                printf("Corrupted cache repository detected, backing up and recreating...\n");
	            }
	            
	            int backup_ret = backup_repository(repo->cache_path, &backup_path, config);
	            if (backup_ret != CACHE_SUCCESS) {
	                fprintf(stderr, "error: failed to backup corrupted repository\n");
	                RETURN_WITH_LOCK_CLEANUP(repo->cache_path, backup_ret);
	            }
	        }
	    } else {
	        /* Directory exists but is not a git repository - remove it */
	        if (config->verbose) {
	            printf("Non-git directory found at cache path, removing...\n");
	        }
	        
	        int remove_ret = safe_remove_directory(repo->cache_path, config);
	        if (remove_ret != CACHE_SUCCESS) {
	            fprintf(stderr, "error: failed to remove non-git directory\n");
	            RETURN_WITH_LOCK_CLEANUP(repo->cache_path, remove_ret);
	        }
	    }
	}
	
	/* Ensure parent directory exists */
	char *parent_dir = malloc(strlen(repo->cache_path) + 1);
	if (!parent_dir) {
	    if (backup_path) {
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_MEMORY);
	}
	strcpy(parent_dir, repo->cache_path);
	char *last_slash = strrchr(parent_dir, '/');
	if (last_slash) {
	    *last_slash = '\0';
	} else {
	    strcpy(parent_dir, ".");
	}
	
	int ensure_ret = ensure_directory_exists(parent_dir);
	if (ensure_ret != CACHE_SUCCESS) {
	    free(parent_dir);
	    if (backup_path) {
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, ensure_ret);
	}
	
	/* Create temporary clone path for atomic operation */
	char *temp_path = malloc(strlen(repo->cache_path) + strlen(".tmp.") + 16);
	if (!temp_path) {
	    free(parent_dir);
	    if (backup_path) {
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_MEMORY);
	}
	
	time_t now = time(NULL);
	snprintf(temp_path, strlen(repo->cache_path) + strlen(".tmp.") + 16,
	         "%s.tmp.%ld", repo->cache_path, (long)now);
	
	/* Check available disk space before cloning (estimate 100MB minimum) */
	if (!check_disk_space(parent_dir, 100)) {
	    fprintf(stderr, "Warning: Low disk space detected in %s\n", parent_dir);
	    if (config->verbose) {
	        printf("Continuing with clone operation despite low disk space...\n");
	    }
	}
	
	/* Create new bare repository in temporary location */
	
	/* Build strategy arguments - cache repository should always be full clone */
	/* IMPORTANT: Shallow repositories cannot be used as reference repositories */
	/* Even if user requests shallow clone, the cache must be full to support --reference */
	char strategy_args[256] = "";
	switch (repo->strategy) {
	    case CLONE_STRATEGY_TREELESS:
	        strcpy(strategy_args, " --filter=tree:0");
	        break;
	    case CLONE_STRATEGY_BLOBLESS:
	        strcpy(strategy_args, " --filter=blob:none");
	        break;
	    case CLONE_STRATEGY_SHALLOW:
	        /* Shallow strategy only applies to checkouts, not cache */
	        /* Fall through to full clone */
	    case CLONE_STRATEGY_FULL:
	    default:
	        /* No additional args for full clone */
	        break;
	}
	
	const char *recursive_args = (config->recursive_submodules) ? " --recurse-submodules" : "";
	size_t cmd_len = strlen("git clone --bare") + strlen(strategy_args) + strlen(recursive_args) + strlen(" \"") + strlen(repo->original_url) + 
	                 strlen("\" \"") + strlen(temp_path) + strlen("\"") + 1;
	char *clone_cmd = malloc(cmd_len);
	if (!clone_cmd) {
	    free(parent_dir);
	    free(temp_path);
	    if (backup_path) {
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_MEMORY);
	}
	
	snprintf(clone_cmd, cmd_len, "git clone --bare%s%s \"%s\" \"%s\"", 
	         strategy_args, recursive_args, repo->original_url, temp_path);
	
	if (config->verbose) {
	    printf("Executing: %s\n", clone_cmd);
	}
	
	/* Use enhanced network retry for clone operation */
	char *full_cmd = malloc(strlen("cd \"") + strlen(parent_dir) + strlen("\" && ") + strlen(clone_cmd) + 1);
	if (!full_cmd) {
	    free(clone_cmd);
	    free(parent_dir);
	    free(temp_path);
	    if (backup_path) {
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_MEMORY);
	}
	
	snprintf(full_cmd, strlen("cd \"") + strlen(parent_dir) + strlen("\" && ") + strlen(clone_cmd) + 1,
	         "cd \"%s\" && %s", parent_dir, clone_cmd);
	
	int result = retry_network_operation_with_progress(full_cmd, 3, config, "Cloning repository");
	free(clone_cmd);
	free(full_cmd);
	free(parent_dir);
	
	if (result != 0) {
	    fprintf(stderr, "error: git clone failed with exit code %d\n", result);
	    fprintf(stderr, "This could be due to:\n");
	    fprintf(stderr, "  - Network connectivity issues\n");
	    fprintf(stderr, "  - invalid repository URL '%s'\n", repo->original_url);
	    fprintf(stderr, "  - authentication required (try setting GITHUB_TOKEN)\n");
	    fprintf(stderr, "  - repository does not exist or is private\n");
	    
	    /* Clean up temporary directory */
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    
	    /* Restore backup if we had one */
	    if (backup_path) {
	        if (config->verbose) {
	            printf("Restoring from backup due to clone failure...\n");
	        }
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_GIT);
	}
	
	/* Validate the newly cloned repository */
	if (!validate_git_repository(temp_path, 1)) {
	    fprintf(stderr, "error: cloned repository failed validation\n");
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    
	    if (backup_path) {
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_GIT);
	}
	
	/* Atomically move temporary repository to final location */
	char *mv_cmd = malloc(strlen("mv \"") + strlen(temp_path) + strlen("\" \"") + 
	                     strlen(repo->cache_path) + strlen("\"") + 1);
	if (!mv_cmd) {
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    if (backup_path) {
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_MEMORY);
	}
	
	snprintf(mv_cmd, strlen("mv \"") + strlen(temp_path) + strlen("\" \"") + 
	         strlen(repo->cache_path) + strlen("\"") + 1,
	         "mv \"%s\" \"%s\"", temp_path, repo->cache_path);
	
	int mv_result = system(mv_cmd);
	free(mv_cmd);
	
	if (WEXITSTATUS(mv_result) != 0) {
	    fprintf(stderr, "error: failed to move repository to final location\n");
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    
	    if (backup_path) {
	        restore_from_backup(backup_path, repo->cache_path, config);
	        free(backup_path);
	    }
	    
	    RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_ERROR_FILESYSTEM);
	}
	
	free(temp_path);
	
	/* Clean up backup on success */
	if (backup_path) {
	    if (config->verbose) {
	        printf("Removing backup after successful clone: %s\n", backup_path);
	    }
	    safe_remove_directory(backup_path, config);
	    free(backup_path);
	}
	
	/* Save metadata for newly cached repository */
	struct cache_metadata *metadata = cache_metadata_create(repo);
	if (metadata) {
	    /* Update sync time since we just cloned/created the repository */
	    metadata->last_sync_time = time(NULL);
	    
	    /* Calculate and store cache size */
	    metadata->cache_size = cache_metadata_calculate_size(repo->cache_path);
	    
	    /* Check if repository has submodules */
	    char *submodule_check_cmd = malloc(strlen("cd \"") + strlen(repo->cache_path) + 
	                                      strlen("\" && git submodule status --quiet 2>/dev/null | wc -l") + 1);
	    if (submodule_check_cmd) {
	        snprintf(submodule_check_cmd, strlen("cd \"") + strlen(repo->cache_path) + 
	                 strlen("\" && git submodule status --quiet 2>/dev/null | wc -l") + 1,
	                 "cd \"%s\" && git submodule status --quiet 2>/dev/null | wc -l", repo->cache_path);
	        
	        FILE *pipe = popen(submodule_check_cmd, "r");
	        if (pipe) {
	            char buffer[32];
	            if (fgets(buffer, sizeof(buffer), pipe)) {
	                int submodule_count = atoi(buffer);
	                metadata->has_submodules = (submodule_count > 0);
	            }
	            pclose(pipe);
	        }
	        free(submodule_check_cmd);
	    }
	    
	    /* Get default branch */
	    char *branch_cmd = malloc(strlen("cd \"") + strlen(repo->cache_path) + 
	                             strlen("\" && git symbolic-ref HEAD 2>/dev/null | sed 's|refs/heads/||'") + 1);
	    if (branch_cmd) {
	        snprintf(branch_cmd, strlen("cd \"") + strlen(repo->cache_path) + 
	                 strlen("\" && git symbolic-ref HEAD 2>/dev/null | sed 's|refs/heads/||'") + 1,
	                 "cd \"%s\" && git symbolic-ref HEAD 2>/dev/null | sed 's|refs/heads/||'", repo->cache_path);
	        
	        FILE *pipe = popen(branch_cmd, "r");
	        if (pipe) {
	            char buffer[256];
	            if (fgets(buffer, sizeof(buffer), pipe)) {
	                /* Remove trailing newline */
	                size_t len = strlen(buffer);
	                if (len > 0 && buffer[len-1] == '\n') {
	                    buffer[len-1] = '\0';
	                }
	                metadata->default_branch = strdup(buffer);
	            }
	            pclose(pipe);
	        }
	        free(branch_cmd);
	    }
	    
	    /* Save metadata */
	    int metadata_ret = cache_metadata_save(repo->cache_path, metadata);
	    if (metadata_ret != METADATA_SUCCESS) {
	        if (config->verbose) {
	            printf("Warning: Failed to save cache metadata (error %d)\n", metadata_ret);
	        }
	    } else if (config->verbose) {
	        printf("Cache metadata saved successfully\n");
	    }
	    
	    cache_metadata_destroy(metadata);
	}
	
	if (config->verbose) {
	    printf("Cache repository created successfully\n");
	}
	
	RETURN_WITH_LOCK_CLEANUP(repo->cache_path, CACHE_SUCCESS);
}

/* Handle GitHub repository forking */
static int handle_github_fork(struct repo_info *repo, const struct cache_config *config, 
	                         const struct cache_options *options)
{
	if (!repo || !config || !options) {
	    return CACHE_ERROR_ARGS;
	}
	
	if (config->verbose) {
	    printf("Creating GitHub fork in organization: %s\n", repo->fork_organization);
	}
	
	/* Create GitHub API client */
	struct github_client *client = github_client_create(config->github_token);
	if (!client) {
	    return CACHE_ERROR_GITHUB;
	}
	
	/* Use fork configuration to create fork */
	struct fork_result result;
	int ret = create_fork_with_config(client, repo->owner, repo->name, 
	                                  (struct fork_config *)config->fork_config, &result);
	
	if (ret == 0 && result.success) {
	    if (config->verbose) {
	        printf("Fork created successfully\n");
	    }
	    
	    /* Store the fork URL for use in modifiable checkout */
	    if (result.fork_url) {
	        repo->fork_url = strdup(result.fork_url);
	    }
	    
	    ret = CACHE_SUCCESS;
	} else if (result.already_exists) {
	    if (config->verbose) {
	        printf("Fork already exists\n");
	    }
	    /* Try to construct the fork URL even if fork already exists */
	    char constructed_url[1024];
	    snprintf(constructed_url, sizeof(constructed_url), 
	             "git@github.com:%s/%s-%s.git", 
	             repo->fork_organization, repo->owner, repo->name);
	    repo->fork_url = strdup(constructed_url);
	    ret = CACHE_SUCCESS; /* Not a fatal error */
	} else {
	    if (config->verbose) {
	        printf("Fork creation failed: %s\n", 
	               result.error_message ? result.error_message : "Unknown error");
	    }
	    ret = CACHE_ERROR_GITHUB;
	}
	
	cleanup_fork_result(&result);
	github_client_destroy(client);
	return ret;
}

/* Create reference-based checkouts */
static int create_reference_checkouts(const struct repo_info *repo, const struct cache_config *config,
	                                 const struct cache_options *options)
{
	if (!repo || !config || !options) {
	    return CACHE_ERROR_ARGS;
	}
	
	/* Create read-only checkout */
	int ret = create_reference_checkout(repo->cache_path, repo->checkout_path, 
	                                   repo->strategy, options, config, repo->original_url);
	if (ret != CACHE_SUCCESS) {
	    return ret;
	}
	
	/* Create modifiable checkout (always use blobless for development) */
	/* Use fork URL if available, otherwise original URL */
	const char *modifiable_url = repo->fork_url ? repo->fork_url : repo->original_url;
	if (config->verbose && repo->fork_url) {
	    printf("Using forked repository for modifiable checkout: %s\n", modifiable_url);
	}
	ret = create_reference_checkout(repo->cache_path, repo->modifiable_path,
	                               CLONE_STRATEGY_BLOBLESS, options, config, modifiable_url);
	if (ret != CACHE_SUCCESS) {
	    return ret;
	}
	
	return CACHE_SUCCESS;
}

/* Create a single reference-based checkout */
static int create_reference_checkout(const char *cache_path, const char *checkout_path,
	                                enum clone_strategy strategy, const struct cache_options *options,
	                                const struct cache_config *config, const char *original_url)
{
	if (!cache_path || !checkout_path || !options || !config || !original_url) {
	    return CACHE_ERROR_ARGS;
	}
	
	/* Validate that cache repository exists and is healthy */
	if (!validate_git_repository(cache_path, 1)) {
	    fprintf(stderr, "error: cache repository is invalid or corrupted: %s\n", cache_path);
	    return CACHE_ERROR_GIT;
	}
	
	/* Acquire lock for checkout repository operations */
	int lock_ret = acquire_lock(checkout_path, config);
	if (lock_ret != CACHE_SUCCESS) {
	    fprintf(stderr, "error: failed to acquire lock for checkout repository\n");
	    return lock_ret;
	}
	
	char *backup_path = NULL;
	
	/* Check if checkout already exists */
	if (directory_exists(checkout_path)) {
	    if (is_git_repository_at(checkout_path)) {
	        /* Validate existing repository */
	        if (validate_git_repository(checkout_path, 0)) {
	            if (config->verbose) {
	                printf("Valid checkout found at: %s, updating...\n", checkout_path);
	            }
	            
	            /* Update the existing checkout */
	            char *pull_cmd = malloc(strlen("git pull --ff-only") + 1);
	            if (!pull_cmd) {
	                RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_MEMORY);
	            }
	            strcpy(pull_cmd, "git pull --ff-only");
	            
	            int result = run_git_command_with_progress(pull_cmd, checkout_path, "Updating checkout repository");
	            free(pull_cmd);
	            
	            if (result != 0) {
	                if (config->verbose) {
	                    printf("Warning: Pull failed with exit code %d, but checkout is still valid\n", result);
	                }
	            }
	            
	            /* Update metadata - update access time for existing checkout */
	            cache_metadata_update_access(cache_path);
	            
	            RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_SUCCESS);
	        } else {
	            /* Repository exists but is corrupted - back it up and recreate */
	            if (config->verbose) {
	                printf("Corrupted checkout detected, backing up and recreating...\n");
	            }
	            
	            int backup_ret = backup_repository(checkout_path, &backup_path, config);
	            if (backup_ret != CACHE_SUCCESS) {
	                fprintf(stderr, "error: failed to backup corrupted checkout\n");
	                RETURN_WITH_LOCK_CLEANUP(checkout_path, backup_ret);
	            }
	        }
	    } else {
	        /* Directory exists but is not a git repository - remove it */
	        if (config->verbose) {
	            printf("Non-git directory found at checkout path, removing...\n");
	        }
	        
	        int remove_ret = safe_remove_directory(checkout_path, config);
	        if (remove_ret != CACHE_SUCCESS) {
	            fprintf(stderr, "error: failed to remove non-git directory\n");
	            RETURN_WITH_LOCK_CLEANUP(checkout_path, remove_ret);
	        }
	    }
	}
	
	/* Ensure parent directory exists */
	char *parent_dir = malloc(strlen(checkout_path) + 1);
	if (!parent_dir) {
	    if (backup_path) {
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_MEMORY);
	}
	strcpy(parent_dir, checkout_path);
	char *last_slash = strrchr(parent_dir, '/');
	if (last_slash) {
	    *last_slash = '\0';
	} else {
	    strcpy(parent_dir, ".");
	}
	
	/* Clean up any orphaned temporary files from previous interrupted operations */
	if (directory_exists(parent_dir)) {
	    char *cleanup_pattern = malloc(strlen(checkout_path) + strlen(".tmp.*") + 1);
	    if (cleanup_pattern) {
	        snprintf(cleanup_pattern, strlen(checkout_path) + strlen(".tmp.*") + 1,
	                 "%s.tmp.*", checkout_path);
	        
	        if (config->verbose) {
	            printf("Cleaning up any orphaned temporary files: %s\n", cleanup_pattern);
	        }
	        
	        char *cleanup_cmd = malloc(strlen("rm -rf ") + strlen(cleanup_pattern) + 1);
	        if (cleanup_cmd) {
	            snprintf(cleanup_cmd, strlen("rm -rf ") + strlen(cleanup_pattern) + 1,
	                     "rm -rf %s", cleanup_pattern);
	            if (system(cleanup_cmd) != 0) {
	                /* Cleanup failed, but continue - this is not critical */
	            }
	            free(cleanup_cmd);
	        }
	        free(cleanup_pattern);
	    }
	}
	
	int ensure_ret = ensure_directory_exists(parent_dir);
	if (ensure_ret != CACHE_SUCCESS) {
	    free(parent_dir);
	    if (backup_path) {
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, ensure_ret);
	}
	
	/* Create temporary checkout path for atomic operation */
	char *temp_path = malloc(strlen(checkout_path) + strlen(".tmp.") + 16);
	if (!temp_path) {
	    free(parent_dir);
	    if (backup_path) {
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_MEMORY);
	}
	
	time_t now = time(NULL);
	snprintf(temp_path, strlen(checkout_path) + strlen(".tmp.") + 16,
	         "%s.tmp.%ld", checkout_path, (long)now);
	
	if (config->verbose) {
	    printf("Creating reference checkout at: %s\n", checkout_path);
	}
	
	/* Build strategy arguments */
	char strategy_args[256] = "";
	switch (strategy) {
	    case CLONE_STRATEGY_SHALLOW:
	        snprintf(strategy_args, sizeof(strategy_args), "--depth=%d", options->depth);
	        break;
	    case CLONE_STRATEGY_TREELESS:
	        strcpy(strategy_args, "--filter=tree:0");
	        break;
	    case CLONE_STRATEGY_BLOBLESS:
	        strcpy(strategy_args, "--filter=blob:none");
	        break;
	    case CLONE_STRATEGY_FULL:
	    default:
	        /* No additional args for full clone */
	        break;
	}
	
	/* Build clone command with precise length calculation */
	const char *recursive_args = (config->recursive_submodules) ? " --recurse-submodules" : "";
	size_t cmd_len = strlen("git clone --reference \"") + strlen(cache_path) + 
	                 strlen("\" ") + strlen(strategy_args) + strlen(recursive_args) + strlen(" \"") + 
	                 strlen(original_url) + strlen("\" \"") + strlen(temp_path) + strlen("\"") + 1;
	char *clone_cmd = malloc(cmd_len);
	if (!clone_cmd) {
	    free(parent_dir);
	    free(temp_path);
	    if (backup_path) {
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_MEMORY);
	}
	
	snprintf(clone_cmd, cmd_len, "git clone --reference \"%s\" %s%s \"%s\" \"%s\"",
	         cache_path, strategy_args, recursive_args, original_url, temp_path);
	
	if (config->verbose) {
	    printf("Executing: %s\n", clone_cmd);
	}
	
	int result = run_git_command(clone_cmd, parent_dir);
	free(clone_cmd);
	free(parent_dir);
	
	if (result != 0) {
	    fprintf(stderr, "error: reference checkout failed with exit code %d\n", result);
	    fprintf(stderr, "This could be due to:\n");
	    fprintf(stderr, "  - Cache repository corruption\n");
	    fprintf(stderr, "  - Network connectivity issues for strategy filters\n");
	    fprintf(stderr, "  - Filesystem permissions\n");
	    fprintf(stderr, "  - Invalid original URL '%s'\n", original_url);
	    
	    /* Clean up temporary directory */
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    
	    /* Restore backup if we had one */
	    if (backup_path) {
	        if (config->verbose) {
	            printf("Restoring from backup due to checkout failure...\n");
	        }
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_GIT);
	}
	
	/* Validate the newly created checkout - check both working tree and .git directory */
	char *git_subdir = malloc(strlen(temp_path) + strlen("/.git") + 1);
	if (!git_subdir) {
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    if (backup_path) {
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_MEMORY);
	}
	snprintf(git_subdir, strlen(temp_path) + strlen("/.git") + 1, "%s/.git", temp_path);
	
	if (!directory_exists(git_subdir) && !validate_git_repository(temp_path, 0)) {
	    fprintf(stderr, "error: created checkout failed validation\n");
	    free(git_subdir);
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    
	    if (backup_path) {
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_GIT);
	}
	free(git_subdir);
	
	/* Atomically move temporary checkout to final location */
	char *mv_cmd = malloc(strlen("mv \"") + strlen(temp_path) + strlen("\" \"") + 
	                     strlen(checkout_path) + strlen("\"") + 1);
	if (!mv_cmd) {
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    if (backup_path) {
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_MEMORY);
	}
	
	snprintf(mv_cmd, strlen("mv \"") + strlen(temp_path) + strlen("\" \"") + 
	         strlen(checkout_path) + strlen("\"") + 1,
	         "mv \"%s\" \"%s\"", temp_path, checkout_path);
	
	int mv_result = system(mv_cmd);
	free(mv_cmd);
	
	if (WEXITSTATUS(mv_result) != 0) {
	    fprintf(stderr, "error: failed to move checkout to final location\n");
	    safe_remove_directory(temp_path, config);
	    free(temp_path);
	    
	    if (backup_path) {
	        restore_from_backup(backup_path, checkout_path, config);
	        free(backup_path);
	    }
	    
	    RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_ERROR_FILESYSTEM);
	}
	
	free(temp_path);
	
	/* Clean up backup on success */
	if (backup_path) {
	    if (config->verbose) {
	        printf("Removing backup after successful checkout: %s\n", backup_path);
	    }
	    safe_remove_directory(backup_path, config);
	    free(backup_path);
	}
	
	if (config->verbose) {
	    printf("Reference checkout created successfully\n");
	}
	
	/* Update metadata - increment reference count and update access time */
	cache_metadata_increment_ref(cache_path);
	
	RETURN_WITH_LOCK_CLEANUP(checkout_path, CACHE_SUCCESS);
}

/* Scan cache directory and show repository information */
static int scan_cache_directory(const char *cache_dir, const struct cache_config *config, 
	                           const struct cache_options *options)
{
	if (!cache_dir || !config || !options) {
	    return CACHE_ERROR_ARGS;
	}
	
	DIR *dir = opendir(cache_dir);
	if (!dir) {
	    printf("  Unable to scan cache directory\n");
	    return CACHE_ERROR_FILESYSTEM;
	}
	
	const struct dirent *entry;
	int repo_count = 0;
	size_t total_cache_size = 0;
	int total_checkouts = 0;
	int strategy_counts[4] = {0}; /* full, shallow, treeless, blobless */
	
	/* Show scanning progress for list and sync operations */
	if (options->operation == CACHE_OP_LIST || options->operation == CACHE_OP_SYNC) {
	    show_progress_indicator("Scanning cache directory", 0);
	}
	
	while ((entry = readdir(dir)) != NULL) {
	    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
	        /* This is a user directory, scan for repositories */
	        size_t user_dir_len = strlen(cache_dir) + 1 + strlen(entry->d_name) + 1;
	        char *user_dir = malloc(user_dir_len);
	        if (!user_dir) {
	            closedir(dir);
	            return CACHE_ERROR_MEMORY;
	        }
	        snprintf(user_dir, user_dir_len, "%s/%s", cache_dir, entry->d_name);
	        
	        DIR *user_dir_handle = opendir(user_dir);
	        if (user_dir_handle) {
	            const struct dirent *repo_entry;
	            while ((repo_entry = readdir(user_dir_handle)) != NULL) {
	                if (repo_entry->d_type == DT_DIR && strcmp(repo_entry->d_name, ".") != 0 && 
	                    strcmp(repo_entry->d_name, "..") != 0) {
	                    
	                    size_t repo_path_len = strlen(user_dir) + 1 + strlen(repo_entry->d_name) + 1;
	                    char *repo_path = malloc(repo_path_len);
	                    if (!repo_path) {
	                        closedir(user_dir_handle);
	                        free(user_dir);
	                        closedir(dir);
	                        return CACHE_ERROR_MEMORY;
	                    }
	                    snprintf(repo_path, repo_path_len, "%s/%s", user_dir, repo_entry->d_name);
	                    
	                    /* Check if it's a git repository */
	                    if (is_git_repository_at(repo_path)) {
	                        repo_count++;
	                        printf("  %s/%s", entry->d_name, repo_entry->d_name);
	                        
	                        /* Load metadata once for both verbose and non-verbose modes */
	                        struct cache_metadata metadata = {0};
	                        int has_metadata = (cache_metadata_load(repo_path, &metadata) == METADATA_SUCCESS);
	                        
	                        /* Update statistics */
	                        if (has_metadata) {
	                            total_cache_size += metadata.cache_size;
	                            total_checkouts += metadata.ref_count;
	                            if (metadata.strategy >= CLONE_STRATEGY_FULL && metadata.strategy <= CLONE_STRATEGY_BLOBLESS) {
	                                strategy_counts[metadata.strategy]++;
	                            }
	                        }
	                        
	                        if (!options->verbose) {
	                            if (has_metadata) {
	                                /* Show basic metadata in non-verbose mode */
	                                if (metadata.cache_size > 0) {
	                                    double size_mb = metadata.cache_size / (1024.0 * 1024.0);
	                                    if (size_mb < 1024.0) {
	                                        printf(" (%.1fM", size_mb);
	                                    } else {
	                                        printf(" (%.1fG", size_mb / 1024.0);
	                                    }
	                                } else {
	                                    printf(" (?");
	                                }
	                                
	                                printf(", %s", 
	                                       metadata.strategy == CLONE_STRATEGY_FULL ? "full" :
	                                       metadata.strategy == CLONE_STRATEGY_SHALLOW ? "shallow" :
	                                       metadata.strategy == CLONE_STRATEGY_TREELESS ? "treeless" :
	                                       metadata.strategy == CLONE_STRATEGY_BLOBLESS ? "blobless" : "unknown");
	                                
	                                if (metadata.ref_count > 0) {
	                                    printf(", %d checkout%s", metadata.ref_count, 
	                                           metadata.ref_count == 1 ? "" : "s");
	                                }
	                                
	                                printf(")");
	                            }
	                        }
	                        
	                        if (options->verbose) {
	                            /* Show detailed repository information */
	                            printf("\n    Cache path: %s", repo_path);
	                            
	                            /* Get repository size */
	                            if (has_metadata && metadata.cache_size > 0) {
	                                /* Use cached size from metadata */
	                                double size_mb = metadata.cache_size / (1024.0 * 1024.0);
	                                if (size_mb < 1024.0) {
	                                    printf("\n    Size: %.1fM", size_mb);
	                                } else {
	                                    printf("\n    Size: %.1fG", size_mb / 1024.0);
	                                }
	                            } else {
	                                /* Fall back to du command */
	                                char *du_cmd = malloc(strlen("du -sh \"") + strlen(repo_path) + strlen("\" 2>/dev/null | cut -f1") + 1);
	                                if (du_cmd) {
	                                    snprintf(du_cmd, strlen("du -sh \"") + strlen(repo_path) + strlen("\" 2>/dev/null | cut -f1") + 1,
	                                             "du -sh \"%s\" 2>/dev/null | cut -f1", repo_path);
	                                    
	                                    FILE *pipe = popen(du_cmd, "r");
	                                    if (pipe) {
	                                        char size_buf[32];
	                                        if (fgets(size_buf, sizeof(size_buf), pipe)) {
	                                            /* Remove newline */
	                                            char *newline = strchr(size_buf, '\n');
	                                            if (newline) *newline = '\0';
	                                            printf("\n    Size: %s", size_buf);
	                                        }
	                                        pclose(pipe);
	                                    }
	                                    free(du_cmd);
	                                }
	                            }
	                            
	                            /* Get sync and access times */
	                            if (has_metadata) {
	                                /* Show metadata information */
	                                char time_buf[64];
	                                const struct tm *tm_info;
	                                
	                                /* Creation time */
	                                if (metadata.created_time > 0) {
	                                    tm_info = localtime(&metadata.created_time);
	                                    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
	                                    printf("\n    Created: %s", time_buf);
	                                }
	                                
	                                /* Last sync time */
	                                if (metadata.last_sync_time > 0) {
	                                    tm_info = localtime(&metadata.last_sync_time);
	                                    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
	                                    printf("\n    Last sync: %s", time_buf);
	                                } else {
	                                    /* Fall back to HEAD modification time */
	                                    char *head_path = malloc(strlen(repo_path) + strlen("/HEAD") + 1);
	                                    if (head_path) {
	                                        snprintf(head_path, strlen(repo_path) + strlen("/HEAD") + 1, "%s/HEAD", repo_path);
	                                        struct stat st;
	                                        if (stat(head_path, &st) == 0) {
	                                            tm_info = localtime(&st.st_mtime);
	                                            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
	                                            printf("\n    Last sync: %s", time_buf);
	                                        }
	                                        free(head_path);
	                                    }
	                                }
	                                
	                                /* Last access time */
	                                if (metadata.last_access_time > 0) {
	                                    tm_info = localtime(&metadata.last_access_time);
	                                    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
	                                    printf("\n    Last access: %s", time_buf);
	                                }
	                                
	                                /* Clone strategy */
	                                printf("\n    Clone strategy: %s",
	                                       metadata.strategy == CLONE_STRATEGY_FULL ? "full" :
	                                       metadata.strategy == CLONE_STRATEGY_SHALLOW ? "shallow" :
	                                       metadata.strategy == CLONE_STRATEGY_TREELESS ? "treeless" :
	                                       metadata.strategy == CLONE_STRATEGY_BLOBLESS ? "blobless" : "unknown");
	                                
	                                /* Reference count */
	                                printf("\n    Active checkouts: %d", metadata.ref_count);
	                                
	                                /* Repository type */
	                                if (metadata.type != REPO_TYPE_UNKNOWN) {
	                                    printf("\n    Type: %s",
	                                           metadata.type == REPO_TYPE_GITHUB ? "GitHub" : "unknown");
	                                }
	                                
	                                /* Fork information */
	                                if (metadata.is_fork_needed) {
	                                    printf("\n    Fork: %s (%s)",
	                                           metadata.fork_organization ? metadata.fork_organization : "yes",
	                                           metadata.is_private_fork ? "private" : "public");
	                                }
	                                
	                                /* Submodules */
	                                if (metadata.has_submodules) {
	                                    printf("\n    Has submodules: yes");
	                                }
	                                
	                                /* Default branch */
	                                if (metadata.default_branch && strlen(metadata.default_branch) > 0) {
	                                    printf("\n    Default branch: %s", metadata.default_branch);
	                                }
	                            } else {
	                                /* Fall back to HEAD modification time only */
	                                char *head_path = malloc(strlen(repo_path) + strlen("/HEAD") + 1);
	                                if (head_path) {
	                                    snprintf(head_path, strlen(repo_path) + strlen("/HEAD") + 1, "%s/HEAD", repo_path);
	                                    struct stat st;
	                                    if (stat(head_path, &st) == 0) {
	                                        char time_buf[64];
	                                        const struct tm *tm_info = localtime(&st.st_mtime);
	                                        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
	                                        printf("\n    Last sync: %s", time_buf);
	                                    }
	                                    free(head_path);
	                                }
	                            }
	                            
	                            /* Get remote URL */
	                            char *remote_cmd = malloc(strlen("cd \"") + strlen(repo_path) + strlen("\" && git config --get remote.origin.url 2>/dev/null") + 1);
	                            if (remote_cmd) {
	                                snprintf(remote_cmd, strlen("cd \"") + strlen(repo_path) + strlen("\" && git config --get remote.origin.url 2>/dev/null") + 1,
	                                         "cd \"%s\" && git config --get remote.origin.url 2>/dev/null", repo_path);
	                                
	                                FILE *pipe = popen(remote_cmd, "r");
	                                if (pipe) {
	                                    char url_buf[512];
	                                    if (fgets(url_buf, sizeof(url_buf), pipe)) {
	                                        /* Remove newline */
	                                        char *newline = strchr(url_buf, '\n');
	                                        if (newline) *newline = '\0';
	                                        printf("\n    Remote URL: %s", url_buf);
	                                    }
	                                    pclose(pipe);
	                                }
	                                free(remote_cmd);
	                            }
	                            
	                            /* Get branch count */
	                            char *branch_cmd = malloc(strlen("cd \"") + strlen(repo_path) + strlen("\" && git branch -r 2>/dev/null | wc -l") + 1);
	                            if (branch_cmd) {
	                                snprintf(branch_cmd, strlen("cd \"") + strlen(repo_path) + strlen("\" && git branch -r 2>/dev/null | wc -l") + 1,
	                                         "cd \"%s\" && git branch -r 2>/dev/null | wc -l", repo_path);
	                                
	                                FILE *pipe = popen(branch_cmd, "r");
	                                if (pipe) {
	                                    char count_buf[16];
	                                    if (fgets(count_buf, sizeof(count_buf), pipe)) {
	                                        int branch_count = atoi(count_buf);
	                                        if (branch_count > 0) {
	                                            printf("\n    Branches: %d", branch_count);
	                                        }
	                                    }
	                                    pclose(pipe);
	                                }
	                                free(branch_cmd);
	                            }
	                            
	                            /* Check for corresponding checkouts */
	                            size_t checkout_path_len = strlen(config->checkout_root) + 1 + 
	                                                       strlen(entry->d_name) + 1 + strlen(repo_entry->d_name) + 1;
	                            char *checkout_path = malloc(checkout_path_len);
	                            if (checkout_path) {
	                                snprintf(checkout_path, checkout_path_len, "%s/%s/%s", config->checkout_root, 
	                                       entry->d_name, repo_entry->d_name);
	                                if (directory_exists(checkout_path)) {
	                                    printf("\n    Checkout: %s", checkout_path);
	                                    
	                                    /* Determine checkout strategy based on git config */
	                                    char *strategy_cmd = malloc(strlen("cd \"") + strlen(checkout_path) + strlen("\" && git config --get remote.origin.fetch 2>/dev/null") + 1);
	                                    if (strategy_cmd) {
	                                        snprintf(strategy_cmd, strlen("cd \"") + strlen(checkout_path) + strlen("\" && git config --get remote.origin.fetch 2>/dev/null") + 1,
	                                                 "cd \"%s\" && git config --get remote.origin.fetch 2>/dev/null", checkout_path);
	                                        
	                                        FILE *pipe = popen(strategy_cmd, "r");
	                                        if (pipe) {
	                                            char fetch_buf[256];
	                                            if (fgets(fetch_buf, sizeof(fetch_buf), pipe)) {
	                                                if (strstr(fetch_buf, "filter=blob:none")) {
	                                                    printf(" (blobless)");
	                                                } else if (strstr(fetch_buf, "filter=tree:0")) {
	                                                    printf(" (treeless)");
	                                                } else if (strstr(fetch_buf, "depth=")) {
	                                                    printf(" (shallow)");
	                                                } else {
	                                                    printf(" (full)");
	                                                }
	                                            }
	                                            pclose(pipe);
	                                        }
	                                        free(strategy_cmd);
	                                    }
	                                } else {
	                                    printf("\n    Checkout: not created");
	                                }
	                                free(checkout_path);
	                            }
	                            
	                            size_t modifiable_path_len = strlen(config->checkout_root) + strlen("/mithro/") +
	                                                         strlen(entry->d_name) + 1 + strlen(repo_entry->d_name) + 1;
	                            char *modifiable_path = malloc(modifiable_path_len);
	                            if (modifiable_path) {
	                                snprintf(modifiable_path, modifiable_path_len, "%s/mithro/%s-%s", config->checkout_root,
	                                       entry->d_name, repo_entry->d_name);
	                                if (directory_exists(modifiable_path)) {
	                                    printf("\n    Modifiable: %s", modifiable_path);
	                                } else {
	                                    printf("\n    Modifiable: not created");
	                                }
	                                free(modifiable_path);
	                            }
	                        }
	                        printf("\n");
	                        
	                        /* Clean up metadata for all cases */
	                        if (has_metadata) {
	                            if (metadata.original_url) free(metadata.original_url);
	                            if (metadata.fork_url) free(metadata.fork_url);
	                            if (metadata.owner) free(metadata.owner);
	                            if (metadata.name) free(metadata.name);
	                            if (metadata.fork_organization) free(metadata.fork_organization);
	                            if (metadata.default_branch) free(metadata.default_branch);
	                        }
	                    }
	                    
	                    free(repo_path);
	                }
	            }
	            closedir(user_dir_handle);
	        }
	        free(user_dir);
	    }
	}
	
	closedir(dir);
	
	/* Clear scanning progress indicator */
	if (options->operation == CACHE_OP_LIST || options->operation == CACHE_OP_SYNC) {
	    clear_progress_indicator();
	}
	
	if (repo_count == 0) {
	    printf("  No cached repositories found\n");
	} else {
	    printf("\nSummary:\n");
	    printf("  Total repositories: %d\n", repo_count);
	    
	    /* Show total cache size */
	    if (total_cache_size > 0) {
	        double size_mb = total_cache_size / (1024.0 * 1024.0);
	        if (size_mb < 1024.0) {
	            printf("  Total cache size: %.1fM\n", size_mb);
	        } else {
	            printf("  Total cache size: %.1fG\n", size_mb / 1024.0);
	        }
	    }
	    
	    /* Show checkout count */
	    if (total_checkouts > 0) {
	        printf("  Active checkouts: %d\n", total_checkouts);
	    }
	    
	    /* Show strategy breakdown */
	    if (strategy_counts[0] + strategy_counts[1] + strategy_counts[2] + strategy_counts[3] > 0) {
	        printf("  Clone strategies:\n");
	        if (strategy_counts[CLONE_STRATEGY_FULL] > 0)
	            printf("    Full: %d\n", strategy_counts[CLONE_STRATEGY_FULL]);
	        if (strategy_counts[CLONE_STRATEGY_SHALLOW] > 0)
	            printf("    Shallow: %d\n", strategy_counts[CLONE_STRATEGY_SHALLOW]);
	        if (strategy_counts[CLONE_STRATEGY_TREELESS] > 0)
	            printf("    Treeless: %d\n", strategy_counts[CLONE_STRATEGY_TREELESS]);
	        if (strategy_counts[CLONE_STRATEGY_BLOBLESS] > 0)
	            printf("    Blobless: %d\n", strategy_counts[CLONE_STRATEGY_BLOBLESS]);
	    }
	}
	
	return CACHE_SUCCESS;
}

/* Cache operations implementation */
static int cache_clone_repository(const char *url, const struct cache_options *options)
{
	if (!url || !options) {
	    return CACHE_ERROR_ARGS;
	}
	
	if (options->verbose) {
	    printf("Cloning repository: %s\n", url);
	}
	
	/* Create and load configuration */
	struct cache_config *config = cache_config_create();
	if (!config) {
	    return CACHE_ERROR_MEMORY;
	}
	
	int ret = cache_config_load(config);
	if (ret != CACHE_SUCCESS) {
	    cache_config_destroy(config);
	    return ret;
	}
	
	/* Override configuration with command line options */
	config->verbose = options->verbose;
	config->force = options->force;
	config->recursive_submodules = options->recursive_submodules;
	
	ret = cache_config_validate(config);
	if (ret != CACHE_SUCCESS) {
	    cache_config_destroy(config);
	    return ret;
	}
	
	/* Parse repository information */
	struct repo_info *repo = repo_info_create();
	if (!repo) {
	    cache_config_destroy(config);
	    return CACHE_ERROR_MEMORY;
	}
	
	ret = repo_info_parse_url(url, repo);
	if (ret != CACHE_SUCCESS) {
	    repo_info_destroy(repo);
	    cache_config_destroy(config);
	    return ret;
	}
	
	/* Set strategy from options */
	repo->strategy = options->strategy;
	
	/* Apply auto-detection if AUTO strategy was selected */
	if (repo->strategy == CLONE_STRATEGY_AUTO) {
		if (config->verbose) {
			printf("Auto-detecting optimal clone strategy...\n");
		}
		int auto_ret = auto_detect_strategy(repo, config);
		if (auto_ret != 0) {
			if (config->verbose) {
				printf("Auto-detection failed, using default strategy\n");
			}
			repo->strategy = config->default_strategy; /* fallback */
		}
	}
	
	/* Set fork organization */
	if (options->organization) {
	    repo->fork_organization = malloc(strlen(options->organization) + 1);
	    if (!repo->fork_organization) {
	        repo_info_destroy(repo);
	        cache_config_destroy(config);
	        return CACHE_ERROR_MEMORY;
	    }
	    strcpy(repo->fork_organization, options->organization);
	} else if (repo->type == REPO_TYPE_GITHUB) {
	    /* Default to mithro-mirrors for GitHub repos */
	    repo->fork_organization = malloc(strlen("mithro-mirrors") + 1);
	    if (!repo->fork_organization) {
	        repo_info_destroy(repo);
	        cache_config_destroy(config);
	        return CACHE_ERROR_MEMORY;
	    }
	    strcpy(repo->fork_organization, "mithro-mirrors");
	}
	
	/* Setup repository paths */
	ret = repo_info_setup_paths(repo, config);
	if (ret != CACHE_SUCCESS) {
	    repo_info_destroy(repo);
	    cache_config_destroy(config);
	    return ret;
	}
	
	if (options->verbose) {
	    printf("Repository paths:\n");
	    printf("  Cache: %s\n", repo->cache_path);
	    printf("  Checkout: %s\n", repo->checkout_path);
	    printf("  Modifiable: %s\n", repo->modifiable_path);
	    printf("  Strategy: %s\n", 
	           repo->strategy == CLONE_STRATEGY_FULL ? "full" :
	           repo->strategy == CLONE_STRATEGY_SHALLOW ? "shallow" :
	           repo->strategy == CLONE_STRATEGY_TREELESS ? "treeless" :
	           repo->strategy == CLONE_STRATEGY_BLOBLESS ? "blobless" : "unknown");
	    if (repo->fork_organization) {
	        printf("  Fork organization: %s\n", repo->fork_organization);
	    }
	}
	
	/* Create necessary directories */
	ret = ensure_directory_exists(repo->cache_path);
	if (ret != CACHE_SUCCESS) {
	    fprintf(stderr, "Failed to create cache directory: %s\n", repo->cache_path);
	    repo_info_destroy(repo);
	    cache_config_destroy(config);
	    return ret;
	}
	
	ret = ensure_directory_exists(repo->checkout_path);
	if (ret != CACHE_SUCCESS) {
	    fprintf(stderr, "Failed to create checkout directory: %s\n", repo->checkout_path);
	    repo_info_destroy(repo);
	    cache_config_destroy(config);
	    return ret;
	}
	
	/* Create directory for modifiable path */
	char *modifiable_dir = malloc(strlen(repo->modifiable_path) + 1);
	if (!modifiable_dir) {
	    repo_info_destroy(repo);
	    cache_config_destroy(config);
	    return CACHE_ERROR_MEMORY;
	}
	strcpy(modifiable_dir, repo->modifiable_path);
	char *last_slash = strrchr(modifiable_dir, '/');
	if (last_slash) {
	    *last_slash = '\0';
	    ret = ensure_directory_exists(modifiable_dir);
	    if (ret != CACHE_SUCCESS) {
	        fprintf(stderr, "Failed to create modifiable directory: %s\n", modifiable_dir);
	        free(modifiable_dir);
	        repo_info_destroy(repo);
	        cache_config_destroy(config);
	        return ret;
	    }
	}
	free(modifiable_dir);
	
	/* Step 1: Create full bare repository in cache */
	ret = create_cache_repository(repo, config);
	if (ret != CACHE_SUCCESS) {
	    repo_info_destroy(repo);
	    cache_config_destroy(config);
	    return ret;
	}
	
	/* Step 2: Handle GitHub forking if needed */
	if (repo->type == REPO_TYPE_GITHUB && config->github_token && config->fork_config) {
	    int fork_needed = needs_fork(repo, (struct fork_config *)config->fork_config);
	    if (fork_needed > 0) {
	        ret = handle_github_fork(repo, config, options);
	        if (ret != CACHE_SUCCESS && options->verbose) {
	            printf("Warning: GitHub fork operation failed: %s\n", cache_get_error_string(ret));
	            printf("Continuing with original repository...\n");
	        }
	    }
	}
	
	/* Step 3: Create reference-based checkouts */
	ret = create_reference_checkouts(repo, config, options);
	if (ret != CACHE_SUCCESS) {
	    repo_info_destroy(repo);
	    cache_config_destroy(config);
	    return ret;
	}
	
	/* Step 4: Process submodules if requested */
	if (config->recursive_submodules) {
	    if (options->verbose) {
	        printf("Processing submodules...\n");
	    }
	    ret = process_submodules(repo, config, 1);
	    if (ret != 0) {
	        fprintf(stderr, "Warning: Some submodules failed to process\n");
	        /* Continue anyway - main clone succeeded */
	    }
	}
	
	if (options->verbose) {
	    printf("Repository caching completed successfully!\n");
	}
	
	repo_info_destroy(repo);
	cache_config_destroy(config);
	return CACHE_SUCCESS;
}

static int cache_status(const struct cache_options *options)
{
	/* Create and load configuration */
	struct cache_config *config = cache_config_create();
	if (!config) {
	    return CACHE_ERROR_MEMORY;
	}
	
	int ret = cache_config_load(config);
	if (ret != CACHE_SUCCESS) {
	    cache_config_destroy(config);
	    return ret;
	}
	
	if (options->verbose) {
	    config->verbose = 1;
	}
	
	printf("Git Cache Status\n");
	printf("================\n\n");
	
	/* Show configuration */
	printf("Configuration:\n");
	printf("  Cache root: %s\n", config->cache_root ? config->cache_root : "not set");
	printf("  Checkout root: %s\n", config->checkout_root ? config->checkout_root : "not set");
	printf("  GitHub token: %s\n", config->github_token ? "configured" : "not configured");
	printf("  Default strategy: %s\n", 
	       config->default_strategy == CLONE_STRATEGY_FULL ? "full" :
	       config->default_strategy == CLONE_STRATEGY_SHALLOW ? "shallow" :
	       config->default_strategy == CLONE_STRATEGY_TREELESS ? "treeless" :
	       config->default_strategy == CLONE_STRATEGY_BLOBLESS ? "blobless" : "unknown");
	printf("\n");
	
	/* Check cache directory status */
	if (!config->cache_root || !directory_exists(config->cache_root)) {
	    printf("Cache directory: not found\n");
	    cache_config_destroy(config);
	    return CACHE_SUCCESS;
	}
	
	/* Scan for cached repositories */
	printf("Cached repositories:\n");
	size_t github_cache_dir_len = strlen(config->cache_root) + strlen("/github.com") + 1;
	char *github_cache_dir = malloc(github_cache_dir_len);
	if (!github_cache_dir) {
	    cache_config_destroy(config);
	    return CACHE_ERROR_MEMORY;
	}
	snprintf(github_cache_dir, github_cache_dir_len, "%s/github.com", config->cache_root);
	
	if (directory_exists(github_cache_dir)) {
	    ret = scan_cache_directory(github_cache_dir, config, options);
	    if (ret != CACHE_SUCCESS) {
	        free(github_cache_dir);
	        cache_config_destroy(config);
	        return ret;
	    }
	} else {
	    printf("  No cached repositories found\n");
	}
	
	free(github_cache_dir);
	cache_config_destroy(config);
	return CACHE_SUCCESS;
}

static int cache_clean(const struct cache_options *options)
{
	/* Create and load configuration */
	struct cache_config *config = cache_config_create();
	if (!config) {
	    return CACHE_ERROR_MEMORY;
	}
	
	int ret = cache_config_load(config);
	if (ret != CACHE_SUCCESS) {
	    cache_config_destroy(config);
	    return ret;
	}
	
	if (options->verbose) {
	    printf("Cleaning git cache...\n");
	    printf("Cache root: %s\n", config->cache_root ? config->cache_root : "not set");
	    printf("Checkout root: %s\n", config->checkout_root ? config->checkout_root : "not set");
	}
	
	if (!options->force) {
	    printf("This will remove all cached repositories and checkouts.\n");
	    printf("Use --force to confirm this action.\n");
	    cache_config_destroy(config);
	    return CACHE_SUCCESS;
	}
	
	/* Remove cache directory */
	if (config->cache_root && directory_exists(config->cache_root)) {
	    if (options->verbose) {
	        printf("Removing cache directory: %s\n", config->cache_root);
	    }
	    
	    size_t cmd_len = strlen("rm -rf \"") + strlen(config->cache_root) + strlen("\"") + 1;
	    char *rm_cmd = malloc(cmd_len);
	    if (!rm_cmd) {
	        cache_config_destroy(config);
	        return CACHE_ERROR_MEMORY;
	    }
	    snprintf(rm_cmd, cmd_len, "rm -rf \"%s\"", config->cache_root);
	    
	    int result = system(rm_cmd);
	    free(rm_cmd);
	    
	    if (WEXITSTATUS(result) != 0) {
	        fprintf(stderr, "error: failed to remove cache directory\n");
	        cache_config_destroy(config);
	        return CACHE_ERROR_FILESYSTEM;
	    }
	}
	
	/* Remove checkout directory */
	if (config->checkout_root && directory_exists(config->checkout_root)) {
	    if (options->verbose) {
	        printf("Removing checkout directory: %s\n", config->checkout_root);
	    }
	    
	    size_t cmd_len = strlen("rm -rf \"") + strlen(config->checkout_root) + strlen("\"") + 1;
	    char *rm_cmd = malloc(cmd_len);
	    if (!rm_cmd) {
	        cache_config_destroy(config);
	        return CACHE_ERROR_MEMORY;
	    }
	    snprintf(rm_cmd, cmd_len, "rm -rf \"%s\"", config->checkout_root);
	    
	    int result = system(rm_cmd);
	    free(rm_cmd);
	    
	    if (WEXITSTATUS(result) != 0) {
	        fprintf(stderr, "error: failed to remove checkout directory\n");
	        cache_config_destroy(config);
	        return CACHE_ERROR_FILESYSTEM;
	    }
	}
	
	if (options->verbose) {
	    printf("Cache cleanup completed successfully\n");
	} else {
	    printf("Cache cleaned\n");
	}
	
	cache_config_destroy(config);
	return CACHE_SUCCESS;
}

static int cache_sync(const struct cache_options *options)
{
	/* Create and load configuration */
	struct cache_config *config = cache_config_create();
	if (!config) {
	    return CACHE_ERROR_MEMORY;
	}
	
	int ret = cache_config_load(config);
	if (ret != CACHE_SUCCESS) {
	    cache_config_destroy(config);
	    return ret;
	}
	
	if (options->verbose) {
	    printf("Synchronizing cached repositories...\n");
	    printf("Cache root: %s\n", config->cache_root ? config->cache_root : "not set");
	}
	
	if (!config->cache_root) {
	    fprintf(stderr, "error: cache root directory not set\n");
	    cache_config_destroy(config);
	    return CACHE_ERROR_CONFIG;
	}
	
	if (!directory_exists(config->cache_root)) {
	    printf("No cache directory found\n");
	    cache_config_destroy(config);
	    return CACHE_SUCCESS;
	}
	
	/* Scan for cached repositories */
	char *github_path = malloc(strlen(config->cache_root) + strlen("/github.com") + 1);
	if (!github_path) {
	    cache_config_destroy(config);
	    return CACHE_ERROR_MEMORY;
	}
	snprintf(github_path, strlen(config->cache_root) + strlen("/github.com") + 1,
	         "%s/github.com", config->cache_root);
	
	if (!directory_exists(github_path)) {
	    printf("No cached repositories found\n");
	    free(github_path);
	    cache_config_destroy(config);
	    return CACHE_SUCCESS;
	}
	
	int synced_count = 0;
	int failed_count = 0;
	
	/* Iterate through cached repositories */
	DIR *github_dir = opendir(github_path);
	if (!github_dir) {
	    printf("Unable to scan cache directory\n");
	    free(github_path);
	    cache_config_destroy(config);
	    return CACHE_ERROR_FILESYSTEM;
	}
	
	const struct dirent *owner_entry;
	while ((owner_entry = readdir(github_dir)) != NULL) {
	    if (strcmp(owner_entry->d_name, ".") == 0 || strcmp(owner_entry->d_name, "..") == 0) {
	        continue;
	    }
	    
	    char *owner_path = malloc(strlen(github_path) + strlen("/") + strlen(owner_entry->d_name) + 1);
	    if (!owner_path) {
	        continue;
	    }
	    snprintf(owner_path, strlen(github_path) + strlen("/") + strlen(owner_entry->d_name) + 1,
	             "%s/%s", github_path, owner_entry->d_name);
	    
	    if (!directory_exists(owner_path)) {
	        free(owner_path);
	        continue;
	    }
	    
	    DIR *owner_dir = opendir(owner_path);
	    if (!owner_dir) {
	        free(owner_path);
	        continue;
	    }
	    
	    const struct dirent *repo_entry;
	    while ((repo_entry = readdir(owner_dir)) != NULL) {
	        if (strcmp(repo_entry->d_name, ".") == 0 || strcmp(repo_entry->d_name, "..") == 0) {
	            continue;
	        }
	        
	        char *repo_path = malloc(strlen(owner_path) + strlen("/") + strlen(repo_entry->d_name) + 1);
	        if (!repo_path) {
	            continue;
	        }
	        snprintf(repo_path, strlen(owner_path) + strlen("/") + strlen(repo_entry->d_name) + 1,
	                 "%s/%s", owner_path, repo_entry->d_name);
	        
	        if (is_git_repository_at(repo_path) && validate_git_repository(repo_path, 1)) {
	            if (options->verbose) {
	                printf("Syncing %s/%s...\n", owner_entry->d_name, repo_entry->d_name);
	            }
	            
	            /* Acquire lock for this repository */
	            int lock_ret = acquire_lock(repo_path, config);
	            if (lock_ret != CACHE_SUCCESS) {
	                if (options->verbose) {
	                    printf("  Skipped (locked by another process)\n");
	                }
	                free(repo_path);
	                continue;
	            }
	            
	            /* Perform git fetch to sync */
	            char *fetch_cmd = malloc(strlen("git fetch --all --prune") + 1);
	            if (!fetch_cmd) {
	                release_lock(repo_path);
	                free(repo_path);
	                continue;
	            }
	            strcpy(fetch_cmd, "git fetch --all --prune");
	            
	            char *progress_msg = malloc(strlen("Syncing ") + strlen(owner_entry->d_name) + 
	                                       strlen("/") + strlen(repo_entry->d_name) + 1);
	            int fetch_result;
	            if (progress_msg) {
	                snprintf(progress_msg, strlen("Syncing ") + strlen(owner_entry->d_name) + 
	                        strlen("/") + strlen(repo_entry->d_name) + 1,
	                        "Syncing %s/%s", owner_entry->d_name, repo_entry->d_name);
	                fetch_result = run_git_command_with_progress(fetch_cmd, repo_path, progress_msg);
	                free(progress_msg);
	            } else {
	                fetch_result = run_git_command(fetch_cmd, repo_path);
	            }
	            free(fetch_cmd);
	            
	            if (fetch_result == 0) {
	                synced_count++;
	                if (options->verbose) {
	                    printf("  ✓ Synchronized\n");
	                }
	            } else {
	                failed_count++;
	                if (options->verbose) {
	                    printf("  ✗ Sync failed (exit code: %d)\n", fetch_result);
	                }
	            }
	            
	            release_lock(repo_path);
	        }
	        
	        free(repo_path);
	    }
	    
	    closedir(owner_dir);
	    free(owner_path);
	}
	
	closedir(github_dir);
	free(github_path);
	
	/* Print summary */
	printf("Cache sync completed:\n");
	printf("  Synchronized: %d repositories\n", synced_count);
	if (failed_count > 0) {
	    printf("  Failed: %d repositories\n", failed_count);
	}
	
	/* Repair outdated checkouts after successful sync */
	if (synced_count > 0) {
	    if (options->verbose) {
	        printf("\nChecking for outdated checkouts...\n");
	    }
	    
	    int repair_count = repair_all_outdated_checkouts(config, 0);
	    if (repair_count > 0) {
	        printf("  Repaired: %d outdated checkouts\n", repair_count);
	    } else if (repair_count == 0) {
	        if (options->verbose) {
	            printf("  All checkouts are up to date\n");
	        }
	    } else {
	        fprintf(stderr, "  Warning: Failed to repair checkouts (error: %d)\n", repair_count);
	    }
	}
	
	cache_config_destroy(config);
	return (failed_count == 0) ? CACHE_SUCCESS : CACHE_ERROR_NETWORK;
}

static int cache_list(const struct cache_options *options)
{
	/* Create and load configuration */
	struct cache_config *config = cache_config_create();
	if (!config) {
	    return CACHE_ERROR_MEMORY;
	}
	
	int ret = cache_config_load(config);
	if (ret != CACHE_SUCCESS) {
	    cache_config_destroy(config);
	    return ret;
	}
	
	if (options->verbose) {
	    config->verbose = 1;
	}
	
	printf("Cached Repositories\n");
	printf("==================\n\n");
	
	/* Check cache directory status */
	if (!config->cache_root || !directory_exists(config->cache_root)) {
	    printf("No cache directory found\n");
	    cache_config_destroy(config);
	    return CACHE_SUCCESS;
	}
	
	/* Scan for cached repositories */
	size_t github_cache_dir_len = strlen(config->cache_root) + strlen("/github.com") + 1;
	char *github_cache_dir = malloc(github_cache_dir_len);
	if (!github_cache_dir) {
	    cache_config_destroy(config);
	    return CACHE_ERROR_MEMORY;
	}
	snprintf(github_cache_dir, github_cache_dir_len, "%s/github.com", config->cache_root);
	
	if (directory_exists(github_cache_dir)) {
	    ret = scan_cache_directory(github_cache_dir, config, options);
	    if (ret != CACHE_SUCCESS) {
	        free(github_cache_dir);
	        cache_config_destroy(config);
	        return ret;
	    }
	} else {
	    printf("No cached repositories found\n");
	}
	
	free(github_cache_dir);
	cache_config_destroy(config);
	return CACHE_SUCCESS;
}

static int cache_verify(const struct cache_options *options)
{
	/* Create and load configuration */
	struct cache_config *config = cache_config_create();
	if (!config) {
	    return CACHE_ERROR_MEMORY;
	}
	
	cache_config_load(config);
	
	if (options->url) {
	    /* Verify specific repository */
	    struct repo_info *repo = repo_info_create();
	    if (!repo) {
	        cache_config_destroy(config);
	        return CACHE_ERROR_MEMORY;
	    }
	    
	    if (repo_info_parse_url(options->url, repo) != 0) {
	        printf("Invalid URL: %s\n", options->url);
	        repo_info_destroy(repo);
	        cache_config_destroy(config);
	        return CACHE_ERROR_ARGS;
	    }
	    
	    repo_info_setup_paths(repo, config);
	    
	    printf("Verifying repository: %s\n", options->url);
	    int result = verify_and_repair_repository(repo, config);
	    
	    if (result == CACHE_RECOVERY_OK) {
	        printf("Repository verification complete: all components are valid\n");
	    } else {
	        printf("Repository verification failed: %s\n", cache_recovery_error_string(result));
	    }
	    
	    repo_info_destroy(repo);
	    cache_config_destroy(config);
	    return (result == CACHE_RECOVERY_OK) ? CACHE_SUCCESS : CACHE_ERROR_FILESYSTEM;
	} else {
	    /* Verify all cached repositories */
	    printf("Verifying all cached repositories...\n");
	    
	    char cache_base[4096];
	    const char *home = getenv("HOME");
	    if (!home) {
	        cache_config_destroy(config);
	        return CACHE_ERROR_CONFIG;
	    }
	    
	    snprintf(cache_base, sizeof(cache_base), "%s/%s", home, CACHE_BASE_DIR);
	    
	    /* Verify cache directory exists */
	    if (access(cache_base, F_OK) != 0) {
	        printf("No cache directory found at: %s\n", cache_base);
	        cache_config_destroy(config);
	        return CACHE_SUCCESS;
	    }
	    
	    /* Walk through cache directory and verify each repository */
	    char github_path[PATH_MAX];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
	    snprintf(github_path, sizeof(github_path), "%s/github.com", cache_base);
#pragma GCC diagnostic pop
	    
	    DIR *github_dir = opendir(github_path);
	    if (!github_dir) {
	        printf("No GitHub repositories cached\n");
	        cache_config_destroy(config);
	        return CACHE_SUCCESS;
	    }
	    
	    int total_repos = 0;
	    int corrupted_repos = 0;
	    
	    const struct dirent *owner_entry;
	    while ((owner_entry = readdir(github_dir)) != NULL) {
	        if (strcmp(owner_entry->d_name, ".") == 0 || strcmp(owner_entry->d_name, "..") == 0) {
	            continue;
	        }
	        
	        char owner_path[PATH_MAX];
	        if (strlen(github_path) + strlen(owner_entry->d_name) + 2 >= PATH_MAX) {
	            fprintf(stderr, "warning: path too long, skipping %s\n", owner_entry->d_name);
	            continue;
	        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
	        snprintf(owner_path, sizeof(owner_path), "%s/%s", github_path, owner_entry->d_name);
#pragma GCC diagnostic pop
	        
	        DIR *owner_dir = opendir(owner_path);
	        if (!owner_dir) {
	            continue;
	        }
	        
	        const struct dirent *repo_entry;
	        while ((repo_entry = readdir(owner_dir)) != NULL) {
	            if (strcmp(repo_entry->d_name, ".") == 0 || strcmp(repo_entry->d_name, "..") == 0) {
	                continue;
	            }
	            
	            char repo_path[PATH_MAX];
	            if (strlen(owner_path) + strlen(repo_entry->d_name) + 2 >= PATH_MAX) {
	                fprintf(stderr, "warning: path too long, skipping %s\n", repo_entry->d_name);
	                continue;
	            }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
	            snprintf(repo_path, sizeof(repo_path), "%s/%s", owner_path, repo_entry->d_name);
#pragma GCC diagnostic pop
	            
	            total_repos++;
	            printf("Checking: %s/%s... ", owner_entry->d_name, repo_entry->d_name);
	            
	            int status = verify_cache_repository(repo_path);
	            if (status == CACHE_RECOVERY_OK) {
	                printf("OK\n");
	            } else {
	                corrupted_repos++;
	                printf("CORRUPTED (%s)\n", cache_recovery_error_string(status));
	            }
	        }
	        closedir(owner_dir);
	    }
	    closedir(github_dir);
	    
	    printf("\nVerification Summary:\n");
	    printf("  Total repositories: %d\n", total_repos);
	    printf("  Corrupted repositories: %d\n", corrupted_repos);
	    
	    if (corrupted_repos > 0) {
	        printf("\nUse 'git-cache verify <url>' to repair specific repositories\n");
	    }
	    
	    cache_config_destroy(config);
	    return (corrupted_repos == 0) ? CACHE_SUCCESS : CACHE_ERROR_FILESYSTEM;
	}
}

static int cache_repair(const struct cache_options *options)
{
	/* Create and load configuration */
	struct cache_config *config = cache_config_create();
	if (!config) {
	    return CACHE_ERROR_MEMORY;
	}
	
	int ret = cache_config_load(config);
	if (ret != CACHE_SUCCESS) {
	    cache_config_destroy(config);
	    return ret;
	}
	
	if (options->verbose) {
	    printf("Repairing outdated checkouts...\n");
	    printf("Cache root: %s\n", config->cache_root ? config->cache_root : "not set");
	    printf("Checkout root: %s\n", config->checkout_root ? config->checkout_root : "not set");
	}
	
	/* Set configuration options from command line */
	config->verbose = options->verbose;
	if (options->force) {
	    config->force = 1;
	}
	
	/* Call the repair function from checkout_repair.h */
	int repaired_count = repair_all_outdated_checkouts(config, options->force);
	
	if (repaired_count < 0) {
	    fprintf(stderr, "error: failed to repair checkouts: %s\n", 
	            checkout_repair_status_string(repaired_count));
	    cache_config_destroy(config);
	    return CACHE_ERROR_FILESYSTEM;
	}
	
	if (repaired_count == 0) {
	    printf("All checkouts are up to date. No repairs needed.\n");
	} else {
	    printf("Successfully repaired %d checkout(s).\n", repaired_count);
	}
	
	cache_config_destroy(config);
	return CACHE_SUCCESS;
}

/* Configuration command implementation */
static int cache_config_command(const struct cache_options *options)
{
	if (!options) {
		return CACHE_ERROR_ARGS;
	}
	
	/* Create and load configuration */
	struct cache_config *config = cache_config_create();
	if (!config) {
		fprintf(stderr, "error: failed to create configuration\n");
		return CACHE_ERROR_MEMORY;
	}
	
	/* Load configuration from files */
	/* TODO: Temporarily disabled due to memory corruption issue
	int load_result = load_configuration(config);
	if (load_result != CONFIG_SUCCESS && options->verbose) {
		printf("Note: No configuration files found, using defaults\n");
	}
	*/
	
	/* Use cache_config_load instead which handles environment variables properly */
	int load_result = cache_config_load(config);
	if (load_result != CACHE_SUCCESS) {
		cache_config_destroy(config);
		return load_result;
	}
	
	/* If no specific action, show current configuration */
	if (!options->url) {
		printf("Git Cache Configuration\n");
		printf("=======================\n\n");
		
		/* Show configuration file locations */
		printf("Configuration file locations (in order of precedence):\n");
		
		char user_config[PATH_MAX];
		if (get_user_config_path(user_config, sizeof(user_config)) == CONFIG_SUCCESS) {
			printf("  User config:   %s %s\n", user_config, 
			       config_file_exists(user_config) ? "(exists)" : "(not found)");
		}
		
		char local_config[PATH_MAX];
		if (get_local_config_path(local_config, sizeof(local_config)) == CONFIG_SUCCESS) {
			printf("  Local config:  %s %s\n", local_config,
			       config_file_exists(local_config) ? "(exists)" : "(not found)");
		}
		
		printf("  System config: %s %s\n", CONFIG_SYSTEM_PATH,
		       config_file_exists(CONFIG_SYSTEM_PATH) ? "(exists)" : "(not found)");
		
		const char *env_config = getenv(CONFIG_ENV_VAR);
		if (env_config) {
			printf("  Env config:    %s %s\n", env_config,
			       config_file_exists(env_config) ? "(exists)" : "(not found)");
		}
		
		printf("\n");
		print_configuration(config);
		
		printf("\nTo create a default configuration file:\n");
		printf("  git-cache config init\n");
		printf("\nTo edit configuration:\n");
		printf("  git-cache config edit\n");
	}
	/* Handle init flag to create default config */
	else if (strcmp(options->url, "init") == 0) {
		char user_config[PATH_MAX];
		if (get_user_config_path(user_config, sizeof(user_config)) != CONFIG_SUCCESS) {
			fprintf(stderr, "error: could not determine user config path\n");
			cache_config_destroy(config);
			return CACHE_ERROR_FILESYSTEM;
		}
		
		if (config_file_exists(user_config) && !options->force) {
			printf("Configuration file already exists: %s\n", user_config);
			printf("Use --force to overwrite\n");
			cache_config_destroy(config);
			return CACHE_ERROR_ARGS;
		}
		
		int create_result = create_default_config(user_config);
		if (create_result != CONFIG_SUCCESS) {
			fprintf(stderr, "error: failed to create configuration file: %s\n",
			        config_get_error_string(create_result));
			cache_config_destroy(config);
			return CACHE_ERROR_FILESYSTEM;
		}
		
		printf("Created default configuration file: %s\n", user_config);
	}
	/* Handle edit flag to open config in editor */
	else if (strcmp(options->url, "edit") == 0) {
		char user_config[PATH_MAX];
		if (get_user_config_path(user_config, sizeof(user_config)) != CONFIG_SUCCESS) {
			fprintf(stderr, "error: could not determine user config path\n");
			cache_config_destroy(config);
			return CACHE_ERROR_FILESYSTEM;
		}
		
		/* Create default config if it doesn't exist */
		if (!config_file_exists(user_config)) {
			printf("Creating default configuration file...\n");
			create_default_config(user_config);
		}
		
		/* Open in editor */
		const char *editor = getenv("EDITOR");
		if (!editor) {
			editor = "nano";  /* Default editor */
		}
		
		char edit_cmd[PATH_MAX + 256];
		snprintf(edit_cmd, sizeof(edit_cmd), "%s \"%s\"", editor, user_config);
		
		printf("Opening configuration file in %s...\n", editor);
		int edit_result = system(edit_cmd);
		if (WEXITSTATUS(edit_result) != 0) {
			fprintf(stderr, "error: editor exited with non-zero status\n");
			cache_config_destroy(config);
			return CACHE_ERROR_FILESYSTEM;
		}
		
		printf("Configuration file saved.\n");
	}
	else {
		fprintf(stderr, "error: unknown config option: %s\n", options->url);
		fprintf(stderr, "Use 'git-cache config' to show current configuration\n");
		cache_config_destroy(config);
		return CACHE_ERROR_ARGS;
	}
	
	cache_config_destroy(config);
	return CACHE_SUCCESS;
}

/* Handle completion command */
static int cache_completion_command(const struct cache_options *options)
{
	if (!options) {
		return CACHE_ERROR_ARGS;
	}
	
	/* Parse completion subcommand from URL field */
	const char *subcommand = options->url;
	
	if (!subcommand) {
		/* Show completion status by default */
		return show_completion_status();
	}
	
	if (strcmp(subcommand, "status") == 0) {
		return show_completion_status();
	} else if (strcmp(subcommand, "install") == 0) {
		return install_shell_completion(SHELL_TYPE_UNKNOWN);
	} else if (strcmp(subcommand, "uninstall") == 0) {
		return uninstall_shell_completion(SHELL_TYPE_UNKNOWN);
	} else if (strcmp(subcommand, "generate") == 0) {
		enum shell_type shell = detect_shell_type();
		if (shell == SHELL_TYPE_UNKNOWN) {
			fprintf(stderr, "error: could not detect shell type\n");
			return CACHE_ERROR_ARGS;
		}
		return generate_completion_script(shell, NULL);
	} else {
		fprintf(stderr, "error: unknown completion command: %s\n", subcommand);
		fprintf(stderr, "Available commands: status, install, uninstall, generate\n");
		return CACHE_ERROR_ARGS;
	}
}

/* Handle mirror command */
static int cache_mirror_command(const struct cache_options *options)
{
	if (!options) {
		return CACHE_ERROR_ARGS;
	}
	
	/* Parse mirror subcommand from URL field */
	const char *subcommand = options->url;
	
	if (!subcommand) {
		fprintf(stderr, "error: mirror command requires a subcommand\n");
		fprintf(stderr, "Available commands: add, remove, list, sync\n");
		return CACHE_ERROR_ARGS;
	}
	
	if (strcmp(subcommand, "list") == 0) {
		printf("Mirror management not yet implemented\n");
		return CACHE_SUCCESS;
	} else if (strcmp(subcommand, "add") == 0) {
		printf("Mirror add not yet implemented\n");
		return CACHE_SUCCESS;
	} else if (strcmp(subcommand, "remove") == 0) {
		printf("Mirror remove not yet implemented\n");
		return CACHE_SUCCESS;
	} else if (strcmp(subcommand, "sync") == 0) {
		printf("Mirror sync not yet implemented\n");
		return CACHE_SUCCESS;
	} else {
		fprintf(stderr, "error: unknown mirror command: %s\n", subcommand);
		fprintf(stderr, "Available commands: add, remove, list, sync\n");
		return CACHE_ERROR_ARGS;
	}
}

/* Main function */
int main(int argc, char *argv[])
{
	struct cache_options options;
	int ret = parse_arguments(argc, argv, &options);
	
	if (ret != CACHE_SUCCESS) {
	    print_usage(argv[0]);
	    return 1;
	}
	
	if (options.help) {
	    print_usage(argv[0]);
	    return 0;
	}
	
	if (options.version) {
	    print_version();
	    return 0;
	}
	
	/* Check if we're in a git repository when needed */
	if (options.operation == CACHE_OP_STATUS || options.operation == CACHE_OP_SYNC) {
	    if (!is_git_repository()) {
	        fprintf(stderr, "Warning: not in a git repository\n");
	    }
	}
	
	if (options.verbose) {
	    printf("Running git-cache with verbose output\n");
	}
	
	/* Execute the requested operation */
	switch (options.operation) {
	    case CACHE_OP_CLONE:
	        ret = cache_clone_repository(options.url, &options);
	        break;
	    case CACHE_OP_STATUS:
	        ret = cache_status(&options);
	        break;
	    case CACHE_OP_CLEAN:
	        ret = cache_clean(&options);
	        break;
	    case CACHE_OP_SYNC:
	        ret = cache_sync(&options);
	        break;
	    case CACHE_OP_LIST:
	        ret = cache_list(&options);
	        break;
	    case CACHE_OP_VERIFY:
	        ret = cache_verify(&options);
	        break;
	    case CACHE_OP_REPAIR:
	        ret = cache_repair(&options);
	        break;
	    case CACHE_OP_CONFIG:
	        ret = cache_config_command(&options);
	        break;
	    case CACHE_OP_MIRROR:
	        ret = cache_mirror_command(&options);
	        break;
	    case CACHE_OP_COMPLETION:
	        ret = cache_completion_command(&options);
	        break;
	    default:
	        fprintf(stderr, "error: unknown operation\n");
	        return 1;
	}
	
	if (ret != CACHE_SUCCESS) {
	    fprintf(stderr, "error: %s\n", cache_get_error_string(ret));
	    return 1;
	}
	
	return 0;
}