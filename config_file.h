#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

/**
 * @file config_file.h
 * @brief Configuration file support for git-cache
 * 
 * This module provides functionality for reading and managing configuration
 * files (.gitcacherc) to customize git-cache behavior and defaults.
 */

#include "git-cache.h"

/* Forward declarations */
struct cache_config;

/**
 * @brief Configuration file locations (in order of precedence)
 */
#define CONFIG_SYSTEM_PATH    "/etc/git-cache/config"
#define CONFIG_USER_FILE      ".gitcacherc"
#define CONFIG_LOCAL_FILE     ".git/gitcacherc"
#define CONFIG_ENV_VAR        "GIT_CACHE_CONFIG"

/**
 * @brief Configuration file error codes
 */
#define CONFIG_SUCCESS           0
#define CONFIG_ERROR_NOT_FOUND  -1
#define CONFIG_ERROR_PARSE      -2
#define CONFIG_ERROR_IO         -3
#define CONFIG_ERROR_MEMORY     -4
#define CONFIG_ERROR_INVALID    -5

/**
 * @brief Configuration sections
 */
enum config_section {
	CONFIG_SECTION_CACHE,
	CONFIG_SECTION_CLONE,
	CONFIG_SECTION_GITHUB,
	CONFIG_SECTION_SYNC,
	CONFIG_SECTION_STRATEGY,
	CONFIG_SECTION_UNKNOWN
};

/**
 * @brief Configuration key-value pair
 */
struct config_entry {
	char *section;
	char *key;
	char *value;
	struct config_entry *next;
};

/**
 * @brief Configuration file data structure
 */
struct config_file {
	char *path;
	struct config_entry *entries;
	time_t last_modified;
};

/**
 * @brief Load configuration from all available sources
 * @param config Cache configuration to populate
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int load_configuration(struct cache_config *config);

/**
 * @brief Load configuration from specific file
 * @param file_path Path to configuration file
 * @param config Cache configuration to populate
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int load_config_file(const char *file_path, struct cache_config *config);

/**
 * @brief Save current configuration to file
 * @param file_path Path to save configuration
 * @param config Cache configuration to save
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int save_config_file(const char *file_path, const struct cache_config *config);

/**
 * @brief Create default configuration file
 * @param file_path Path where to create default config
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int create_default_config(const char *file_path);

/**
 * @brief Get configuration value as string
 * @param section Configuration section name
 * @param key Configuration key name
 * @param default_value Default value if not found
 * @return Configuration value or default
 */
const char* get_config_string(const char *section, const char *key, const char *default_value);

/**
 * @brief Get configuration value as integer
 * @param section Configuration section name
 * @param key Configuration key name
 * @param default_value Default value if not found
 * @return Configuration value or default
 */
int get_config_int(const char *section, const char *key, int default_value);

/**
 * @brief Get configuration value as boolean
 * @param section Configuration section name
 * @param key Configuration key name
 * @param default_value Default value if not found
 * @return Configuration value or default
 */
int get_config_bool(const char *section, const char *key, int default_value);

/**
 * @brief Set configuration value
 * @param section Configuration section name
 * @param key Configuration key name
 * @param value Configuration value
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int set_config_value(const char *section, const char *key, const char *value);

/**
 * @brief Get user's configuration file path
 * @param path Buffer to store path (should be PATH_MAX size)
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int get_user_config_path(char *path, size_t path_size);

/**
 * @brief Get local (repository) configuration file path
 * @param path Buffer to store path (should be PATH_MAX size)
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int get_local_config_path(char *path, size_t path_size);

/**
 * @brief Check if configuration file exists
 * @param file_path Path to configuration file
 * @return 1 if exists, 0 if not, negative on error
 */
int config_file_exists(const char *file_path);

/**
 * @brief Reload configuration if files have changed
 * @param config Cache configuration to update
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int reload_config_if_changed(struct cache_config *config);

/**
 * @brief Parse configuration file content
 * @param content File content to parse
 * @param config_file Configuration structure to populate
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int parse_config_content(const char *content, struct config_file *config_file);

/**
 * @brief Apply configuration entries to cache config
 * @param entries Configuration entries list
 * @param config Cache configuration to update
 * @return CONFIG_SUCCESS on success, error code on failure
 */
int apply_config_entries(struct config_entry *entries, struct cache_config *config);

/**
 * @brief Clean up configuration file structure
 * @param config_file Configuration file to clean up
 */
void cleanup_config_file(struct config_file *config_file);

/**
 * @brief Clean up configuration entries list
 * @param entries Configuration entries to clean up
 */
void cleanup_config_entries(struct config_entry *entries);

/**
 * @brief Get configuration error message
 * @param error_code Configuration error code
 * @return Human-readable error message
 */
const char* config_get_error_string(int error_code);

/**
 * @brief Print current configuration values
 * @param config Cache configuration to print
 */
void print_configuration(const struct cache_config *config);

/**
 * @brief Validate configuration values
 * @param config Cache configuration to validate
 * @return CONFIG_SUCCESS if valid, error code if invalid
 */
int validate_configuration(const struct cache_config *config);

#endif /* CONFIG_FILE_H */