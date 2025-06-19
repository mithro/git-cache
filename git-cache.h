#ifndef GIT_CACHE_H
#define GIT_CACHE_H

#include <stddef.h>

#define VERSION "1.0.0"
#define PROGRAM_NAME "git-cache"

/* Cache configuration - defaults to project directory */
#define CACHE_BASE_DIR ".cache/git"
#define CHECKOUT_BASE_DIR "github"
#define MODIFIABLE_BASE_DIR "github/mithro"

/* Repository types */
enum repo_type {
    REPO_TYPE_GITHUB,
    REPO_TYPE_UNKNOWN
};

/* Clone strategies */
enum clone_strategy {
    CLONE_STRATEGY_FULL,
    CLONE_STRATEGY_SHALLOW,
    CLONE_STRATEGY_TREELESS,
    CLONE_STRATEGY_BLOBLESS
};

/* Cache operation modes */
enum cache_operation {
    CACHE_OP_CLONE,
    CACHE_OP_STATUS,
    CACHE_OP_CLEAN,
    CACHE_OP_SYNC,
    CACHE_OP_LIST
};

/* Repository information */
struct repo_info {
    char *original_url;
    char *owner;
    char *name;
    char *cache_path;
    char *checkout_path;
    char *modifiable_path;
    enum repo_type type;
    enum clone_strategy strategy;
    int is_fork_needed;
    char *fork_organization;
};

/* Cache configuration */
struct cache_config {
    char *cache_root;
    char *checkout_root;
    char *github_token;
    enum clone_strategy default_strategy;
    int verbose;
    int force;
    int recursive_submodules;
};

/* Command line options */
struct cache_options {
    enum cache_operation operation;
    char *url;
    char *target_path;
    enum clone_strategy strategy;
    int depth;
    int verbose;
    int force;
    int help;
    int version;
    int recursive_submodules;
    char *organization;
    int make_private;
};

/* Function prototypes */

/* Argument parsing */
int parse_arguments(int argc, char *argv[], struct cache_options *options);
void print_usage(const char *program_name);
void print_version(void);

/* Configuration management */
struct cache_config* cache_config_create(void);
void cache_config_destroy(struct cache_config *config);
int cache_config_load(struct cache_config *config);
int cache_config_validate(const struct cache_config *config);

/* Repository management */
struct repo_info* repo_info_create(void);
void repo_info_destroy(struct repo_info *repo);
int repo_info_parse_url(const char *url, struct repo_info *repo);
int repo_info_setup_paths(struct repo_info *repo, const struct cache_config *config);

/* Cache operations */
int cache_clone_repository(const char *url, const struct cache_options *options);
int cache_status(const struct cache_options *options);
int cache_clean(const struct cache_options *options);
int cache_sync(const struct cache_options *options);
int cache_list(const struct cache_options *options);

/* Utility functions */
int ensure_directory_exists(const char *path);
int is_directory_empty(const char *path);
char* resolve_path(const char *path);
char* get_current_directory(void);
char* get_home_directory(void);

/* Error codes */
#define CACHE_SUCCESS          0
#define CACHE_ERROR_ARGS       -1
#define CACHE_ERROR_CONFIG     -2
#define CACHE_ERROR_NETWORK    -3
#define CACHE_ERROR_FILESYSTEM -4
#define CACHE_ERROR_GIT        -5
#define CACHE_ERROR_GITHUB     -6
#define CACHE_ERROR_MEMORY     -7

const char* cache_get_error_string(int error_code);

#endif /* GIT_CACHE_H */