#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
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

/* Placeholder implementation for cache operations */
int cache_clone_repository(const char *url, const struct cache_options *options)
{
    printf("Cache clone: %s\n", url);
    if (options->verbose) {
        printf("  Strategy: %d\n", options->strategy);
        printf("  Depth: %d\n", options->depth);
        printf("  Organization: %s\n", options->organization ? options->organization : "none");
        printf("  Private: %s\n", options->make_private ? "yes" : "no");
        printf("  Recursive: %s\n", options->recursive_submodules ? "yes" : "no");
    }
    
    /* TODO: Implement actual caching logic */
    printf("TODO: Implement cache clone functionality\n");
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