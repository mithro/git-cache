#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <libgen.h>

#include "git-cache.h"
#include "github_api.h"

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
    printf("\n");
    printf("Options:\n");
    printf("    -h, --help         Show this help message\n");
    printf("    -v, --verbose      Enable verbose output\n");
    printf("    -V, --version      Show version information\n");
    printf("    -f, --force        Force operation\n");
    printf("    --strategy <type>  Clone strategy (full, shallow, treeless, blobless)\n");
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
void print_version(void)
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
    }
    
    return CLONE_STRATEGY_FULL;
}

/* Parse command line arguments */
int parse_arguments(int argc, char *argv[], struct cache_options *options)
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
                fprintf(stderr, "Error: --strategy requires an argument\n");
                return CACHE_ERROR_ARGS;
            }
            options->strategy = parse_strategy(argv[i + 1]);
            i++; /* Skip the strategy argument */
        } else if (strcmp(argv[i], "--depth") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --depth requires an argument\n");
                return CACHE_ERROR_ARGS;
            }
            options->depth = atoi(argv[i + 1]);
            if (options->depth <= 0) {
                fprintf(stderr, "Error: depth must be positive\n");
                return CACHE_ERROR_ARGS;
            }
            i++; /* Skip the depth argument */
        } else if (strcmp(argv[i], "--org") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --org requires an argument\n");
                return CACHE_ERROR_ARGS;
            }
            options->organization = argv[i + 1];
            i++; /* Skip the organization argument */
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: unknown option: %s\n", argv[i]);
            return CACHE_ERROR_ARGS;
        } else {
            /* Non-option argument - should be URL or target path */
            if (!options->url) {
                options->url = argv[i];
            } else if (!options->target_path) {
                options->target_path = argv[i];
            } else {
                fprintf(stderr, "Error: too many arguments\n");
                return CACHE_ERROR_ARGS;
            }
        }
        i++;
    }
    
    /* Validate arguments based on operation */
    if (options->operation == CACHE_OP_CLONE && !options->url) {
        fprintf(stderr, "Error: clone operation requires a URL\n");
        return CACHE_ERROR_ARGS;
    }
    
    return CACHE_SUCCESS;
}

/* Get error string for error code */
const char* cache_get_error_string(int error_code)
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

/* Get home directory */
char* get_home_directory(void)
{
    char *home = getenv("HOME");
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
char* resolve_path(const char *path)
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
int ensure_directory_exists(const char *path)
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
    
    char *dir = dirname(path_copy);
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

/* Check if directory is empty */
int is_directory_empty(const char *path)
{
    if (!directory_exists(path)) {
        return 1; /* Non-existent directory is considered empty */
    }
    
    /* TODO: Implement directory content checking */
    return 0;
}

/* Configuration management */

/* Create cache configuration with defaults */
struct cache_config* cache_config_create(void)
{
    struct cache_config *config = malloc(sizeof(struct cache_config));
    if (!config) {
        return NULL;
    }
    
    memset(config, 0, sizeof(struct cache_config));
    
    /* Set default paths */
    char *home = get_home_directory();
    if (home) {
        size_t cache_len = strlen(home) + strlen(CACHE_BASE_DIR) + 1;
        config->cache_root = malloc(cache_len);
        if (config->cache_root) {
            snprintf(config->cache_root, cache_len, "%s%s", home, CACHE_BASE_DIR);
        }
        
        size_t checkout_len = strlen(home) + strlen(CHECKOUT_BASE_DIR) + 1;
        config->checkout_root = malloc(checkout_len);
        if (config->checkout_root) {
            snprintf(config->checkout_root, checkout_len, "%s%s", home, CHECKOUT_BASE_DIR);
        }
        
        free(home);
    }
    
    /* Set default strategy */
    config->default_strategy = CLONE_STRATEGY_TREELESS;
    config->verbose = 0;
    config->force = 0;
    config->recursive_submodules = 1;
    
    /* Get GitHub token from environment */
    char *token = getenv("GITHUB_TOKEN");
    if (token) {
        config->github_token = malloc(strlen(token) + 1);
        if (config->github_token) {
            strcpy(config->github_token, token);
        }
    }
    
    return config;
}

/* Destroy cache configuration */
void cache_config_destroy(struct cache_config *config)
{
    if (!config) {
        return;
    }
    
    free(config->cache_root);
    free(config->checkout_root);
    free(config->github_token);
    free(config);
}

/* Load configuration from environment and config files */
int cache_config_load(struct cache_config *config)
{
    if (!config) {
        return CACHE_ERROR_ARGS;
    }
    
    /* Override with environment variables if set */
    char *env_cache_root = getenv("GIT_CACHE_ROOT");
    if (env_cache_root) {
        free(config->cache_root);
        config->cache_root = resolve_path(env_cache_root);
        if (!config->cache_root) {
            return CACHE_ERROR_MEMORY;
        }
    }
    
    char *env_checkout_root = getenv("GIT_CHECKOUT_ROOT");
    if (env_checkout_root) {
        free(config->checkout_root);
        config->checkout_root = resolve_path(env_checkout_root);
        if (!config->checkout_root) {
            return CACHE_ERROR_MEMORY;
        }
    }
    
    char *env_token = getenv("GITHUB_TOKEN");
    if (env_token) {
        free(config->github_token);
        config->github_token = malloc(strlen(env_token) + 1);
        if (!config->github_token) {
            return CACHE_ERROR_MEMORY;
        }
        strcpy(config->github_token, env_token);
    }
    
    return CACHE_SUCCESS;
}

/* Validate cache configuration */
int cache_config_validate(const struct cache_config *config)
{
    if (!config) {
        return CACHE_ERROR_ARGS;
    }
    
    if (!config->cache_root) {
        fprintf(stderr, "Error: cache root directory not set\n");
        return CACHE_ERROR_CONFIG;
    }
    
    if (!config->checkout_root) {
        fprintf(stderr, "Error: checkout root directory not set\n");
        return CACHE_ERROR_CONFIG;
    }
    
    /* Ensure cache and checkout directories exist */
    int ret = ensure_directory_exists(config->cache_root);
    if (ret != CACHE_SUCCESS) {
        fprintf(stderr, "Error: failed to create cache directory: %s\n", config->cache_root);
        return ret;
    }
    
    ret = ensure_directory_exists(config->checkout_root);
    if (ret != CACHE_SUCCESS) {
        fprintf(stderr, "Error: failed to create checkout directory: %s\n", config->checkout_root);
        return ret;
    }
    
    return CACHE_SUCCESS;
}

/* Repository information management */

/* Create repository information structure */
struct repo_info* repo_info_create(void)
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
        repo->is_fork_needed = 1; /* Default to forking for GitHub repos */
        return CACHE_SUCCESS;
    }
    
    /* If GitHub parsing failed, it's an unknown repository type */
    repo->type = REPO_TYPE_UNKNOWN;
    fprintf(stderr, "Warning: unsupported repository URL format: %s\n", url);
    
    return CACHE_ERROR_ARGS;
}

/* Setup paths for repository based on configuration */
int repo_info_setup_paths(struct repo_info *repo, const struct cache_config *config)
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
    
    /* Setup modifiable path: ~/github/mithro/owner-repo */
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
static int create_reference_checkout(const char *cache_path, const char *checkout_path,
                                    enum clone_strategy strategy, const struct cache_options *options,
                                    const struct cache_config *config);

/* Git operation helpers */

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

/* Check if a git repository exists at the given path */
static int is_git_repository_at(const char *path)
{
    if (!path) {
        return 0;
    }
    
    char *check_cmd = malloc(strlen("git -C \"") + strlen(path) + strlen("\" rev-parse --git-dir >/dev/null 2>&1") + 1);
    if (!check_cmd) {
        return 0;
    }
    
    snprintf(check_cmd, strlen("git -C \"") + strlen(path) + strlen("\" rev-parse --git-dir >/dev/null 2>&1") + 1,
             "git -C \"%s\" rev-parse --git-dir >/dev/null 2>&1", path);
    
    int result = system(check_cmd);
    free(check_cmd);
    return WEXITSTATUS(result) == 0;
}

/* Create full bare repository in cache location */
static int create_cache_repository(struct repo_info *repo, const struct cache_config *config)
{
    if (!repo || !config) {
        return CACHE_ERROR_ARGS;
    }
    
    if (config->verbose) {
        printf("Creating cache repository at: %s\n", repo->cache_path);
    }
    
    /* Check if cache already exists */
    if (is_git_repository_at(repo->cache_path)) {
        if (config->verbose) {
            printf("Cache repository already exists, updating...\n");
        }
        
        /* Update existing repository */
        char *fetch_cmd = malloc(strlen("git fetch --all --prune") + 1);
        if (!fetch_cmd) {
            return CACHE_ERROR_MEMORY;
        }
        strcpy(fetch_cmd, "git fetch --all --prune");
        
        int result = run_git_command(fetch_cmd, repo->cache_path);
        free(fetch_cmd);
        
        if (result != 0) {
            if (config->verbose) {
                printf("Warning: git fetch failed with exit code %d\n", result);
            }
            return CACHE_ERROR_GIT;
        }
        
        return CACHE_SUCCESS;
    }
    
    /* Create new bare repository */
    size_t cmd_len = strlen("git clone --bare \"") + strlen(repo->original_url) + 
                     strlen("\" \"") + strlen(repo->cache_path) + strlen("\"") + 1;
    char *clone_cmd = malloc(cmd_len);
    if (!clone_cmd) {
        return CACHE_ERROR_MEMORY;
    }
    
    snprintf(clone_cmd, cmd_len, "git clone --bare \"%s\" \"%s\"", 
             repo->original_url, repo->cache_path);
    
    if (config->verbose) {
        printf("Executing: %s\n", clone_cmd);
    }
    
    int result = system(clone_cmd);
    free(clone_cmd);
    
    if (WEXITSTATUS(result) != 0) {
        fprintf(stderr, "Error: git clone failed with exit code %d\n", WEXITSTATUS(result));
        return CACHE_ERROR_GIT;
    }
    
    if (config->verbose) {
        printf("Cache repository created successfully\n");
    }
    
    return CACHE_SUCCESS;
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
    
    /* Fork the repository */
    struct github_repo *forked_repo;
    int ret = github_fork_repo(client, repo->owner, repo->name, repo->fork_organization, &forked_repo);
    
    if (ret == GITHUB_SUCCESS) {
        if (config->verbose) {
            printf("Fork created: %s\n", forked_repo->full_name);
        }
        
        /* Set fork to private if requested */
        if (options->make_private) {
            ret = github_set_repo_private(client, forked_repo->owner, forked_repo->name, 1);
            if (ret == GITHUB_SUCCESS) {
                if (config->verbose) {
                    printf("Fork set to private\n");
                }
            } else {
                if (config->verbose) {
                    printf("Warning: failed to set fork to private: %s\n", github_get_error_string(ret));
                }
            }
        }
        
        github_repo_destroy(forked_repo);
    } else if (ret == GITHUB_ERROR_INVALID) {
        if (config->verbose) {
            printf("Fork already exists or validation error\n");
        }
        ret = CACHE_SUCCESS; /* Not a fatal error */
    } else {
        if (config->verbose) {
            printf("Fork creation failed: %s\n", github_get_error_string(ret));
        }
    }
    
    github_client_destroy(client);
    return ret;
}

/* Create reference-based checkouts */
static int create_reference_checkouts(struct repo_info *repo, const struct cache_config *config,
                                     const struct cache_options *options)
{
    if (!repo || !config || !options) {
        return CACHE_ERROR_ARGS;
    }
    
    /* Create read-only checkout */
    int ret = create_reference_checkout(repo->cache_path, repo->checkout_path, 
                                       repo->strategy, options, config);
    if (ret != CACHE_SUCCESS) {
        return ret;
    }
    
    /* Create modifiable checkout (always use blobless for development) */
    ret = create_reference_checkout(repo->cache_path, repo->modifiable_path,
                                   CLONE_STRATEGY_BLOBLESS, options, config);
    if (ret != CACHE_SUCCESS) {
        return ret;
    }
    
    return CACHE_SUCCESS;
}

/* Create a single reference-based checkout */
static int create_reference_checkout(const char *cache_path, const char *checkout_path,
                                    enum clone_strategy strategy, const struct cache_options *options,
                                    const struct cache_config *config)
{
    if (!cache_path || !checkout_path || !options || !config) {
        return CACHE_ERROR_ARGS;
    }
    
    /* Check if checkout already exists */
    if (is_git_repository_at(checkout_path)) {
        if (config->verbose) {
            printf("Checkout already exists at: %s\n", checkout_path);
        }
        return CACHE_SUCCESS;
    }
    
    if (config->verbose) {
        printf("Creating reference checkout at: %s\n", checkout_path);
    }
    
    /* Build clone command with reference and strategy */
    size_t cmd_len = strlen("git clone --reference \"") + strlen(cache_path) + 
                     strlen("\" ") + 100 + strlen(cache_path) + strlen(" \"") + 
                     strlen(checkout_path) + strlen("\"") + 1;
    char *clone_cmd = malloc(cmd_len);
    if (!clone_cmd) {
        return CACHE_ERROR_MEMORY;
    }
    
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
    
    snprintf(clone_cmd, cmd_len, "git clone --reference \"%s\" %s \"%s\" \"%s\"",
             cache_path, strategy_args, cache_path, checkout_path);
    
    if (config->verbose) {
        printf("Executing: %s\n", clone_cmd);
    }
    
    int result = system(clone_cmd);
    free(clone_cmd);
    
    if (WEXITSTATUS(result) != 0) {
        fprintf(stderr, "Error: reference checkout failed with exit code %d\n", WEXITSTATUS(result));
        return CACHE_ERROR_GIT;
    }
    
    if (config->verbose) {
        printf("Reference checkout created successfully\n");
    }
    
    return CACHE_SUCCESS;
}

/* Cache operations implementation */
int cache_clone_repository(const char *url, const struct cache_options *options)
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
    if (repo->type == REPO_TYPE_GITHUB && repo->is_fork_needed && config->github_token) {
        ret = handle_github_fork(repo, config, options);
        if (ret != CACHE_SUCCESS && options->verbose) {
            printf("Warning: GitHub fork operation failed: %s\n", cache_get_error_string(ret));
            printf("Continuing with original repository...\n");
        }
    }
    
    /* Step 3: Create reference-based checkouts */
    ret = create_reference_checkouts(repo, config, options);
    if (ret != CACHE_SUCCESS) {
        repo_info_destroy(repo);
        cache_config_destroy(config);
        return ret;
    }
    
    if (options->verbose) {
        printf("Repository caching completed successfully!\n");
    }
    
    repo_info_destroy(repo);
    cache_config_destroy(config);
    return CACHE_SUCCESS;
}

int cache_status(const struct cache_options *options)
{
    printf("Cache status:\n");
    if (options->verbose) {
        printf("  Verbose mode enabled\n");
    }
    
    /* TODO: Implement cache status */
    printf("TODO: Implement cache status functionality\n");
    return CACHE_SUCCESS;
}

int cache_clean(const struct cache_options *options)
{
    printf("Cache clean:\n");
    if (options->verbose) {
        printf("  Verbose mode enabled\n");
    }
    if (options->force) {
        printf("  Force mode enabled\n");
    }
    
    /* TODO: Implement cache cleanup */
    printf("TODO: Implement cache clean functionality\n");
    return CACHE_SUCCESS;
}

int cache_sync(const struct cache_options *options)
{
    printf("Cache sync:\n");
    if (options->verbose) {
        printf("  Verbose mode enabled\n");
    }
    
    /* TODO: Implement cache sync */
    printf("TODO: Implement cache sync functionality\n");
    return CACHE_SUCCESS;
}

int cache_list(const struct cache_options *options)
{
    printf("Cache list:\n");
    if (options->verbose) {
        printf("  Verbose mode enabled\n");
    }
    
    /* TODO: Implement cache list */
    printf("TODO: Implement cache list functionality\n");
    return CACHE_SUCCESS;
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
        default:
            fprintf(stderr, "Error: unknown operation\n");
            return 1;
    }
    
    if (ret != CACHE_SUCCESS) {
        fprintf(stderr, "Error: %s\n", cache_get_error_string(ret));
        return 1;
    }
    
    return 0;
}