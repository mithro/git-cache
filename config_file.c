/**
 * @file config_file.c
 * @brief Implementation of configuration file support
 * 
 * Provides functionality for reading and managing configuration files
 * (.gitcacherc) to customize git-cache behavior and defaults.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <pwd.h>

#include "git-cache.h"
#include "config_file.h"

/* Global configuration storage */
static struct config_file global_config = {0};

/**
 * @brief Get user's home directory
 */
static const char* get_home_directory(void)
{
	const char *home = getenv("HOME");
	if (home) {
		return home;
	}
	
	struct passwd *pw = getpwuid(getuid());
	if (pw) {
		return pw->pw_dir;
	}
	
	return NULL;
}

/**
 * @brief Get user's configuration file path
 */
int get_user_config_path(char *path, size_t path_size)
{
	if (!path || path_size == 0) {
		return CONFIG_ERROR_INVALID;
	}
	
	const char *home = get_home_directory();
	if (!home) {
		return CONFIG_ERROR_NOT_FOUND;
	}
	
	int ret = snprintf(path, path_size, "%s/%s", home, CONFIG_USER_FILE);
	if (ret < 0 || (size_t)ret >= path_size) {
		return CONFIG_ERROR_INVALID;
	}
	
	return CONFIG_SUCCESS;
}

/**
 * @brief Get local (repository) configuration file path
 */
int get_local_config_path(char *path, size_t path_size)
{
	if (!path || path_size == 0) {
		return CONFIG_ERROR_INVALID;
	}
	
	/* Check if we're in a git repository */
	if (access(".git", F_OK) == 0) {
		int ret = snprintf(path, path_size, "%s", CONFIG_LOCAL_FILE);
		if (ret < 0 || (size_t)ret >= path_size) {
			return CONFIG_ERROR_INVALID;
		}
		return CONFIG_SUCCESS;
	}
	
	return CONFIG_ERROR_NOT_FOUND;
}

/**
 * @brief Check if configuration file exists
 */
int config_file_exists(const char *file_path)
{
	if (!file_path) {
		return -1;
	}
	
	return access(file_path, F_OK) == 0 ? 1 : 0;
}

/**
 * @brief Trim whitespace from string
 */
static char* trim_whitespace(char *str)
{
	if (!str) {
		return str;
	}
	
	/* Trim leading whitespace */
	while (isspace(*str)) {
		str++;
	}
	
	/* Trim trailing whitespace */
	char *end = str + strlen(str) - 1;
	while (end > str && isspace(*end)) {
		*end = '\0';
		end--;
	}
	
	return str;
}

/**
 * @brief Parse section name from line like [section]
 */
static char* parse_section_name(const char *line)
{
	if (!line) {
		return NULL;
	}
	
	const char *start = strchr(line, '[');
	if (!start) {
		return NULL;
	}
	start++;
	
	const char *end = strchr(start, ']');
	if (!end) {
		return NULL;
	}
	
	size_t len = end - start;
	char *section = malloc(len + 1);
	if (!section) {
		return NULL;
	}
	
	strncpy(section, start, len);
	section[len] = '\0';
	
	char *trimmed = trim_whitespace(section);
	if (trimmed != section) {
		/* trim_whitespace returned a different pointer, need to copy */
		char *result = strdup(trimmed);
		free(section);
		return result;
	}
	
	return section;
}

/**
 * @brief Parse key-value pair from line like key = value
 */
static int parse_key_value(const char *line, char **key, char **value)
{
	if (!line || !key || !value) {
		return CONFIG_ERROR_INVALID;
	}
	
	*key = NULL;
	*value = NULL;
	
	const char *equals = strchr(line, '=');
	if (!equals) {
		return CONFIG_ERROR_PARSE;
	}
	
	/* Extract key */
	size_t key_len = equals - line;
	char *key_buf = malloc(key_len + 1);
	if (!key_buf) {
		return CONFIG_ERROR_MEMORY;
	}
	
	strncpy(key_buf, line, key_len);
	key_buf[key_len] = '\0';
	char *trimmed_key = trim_whitespace(key_buf);
	
	/* Copy the trimmed key to a new buffer */
	*key = strdup(trimmed_key);
	free(key_buf);
	if (!*key) {
		return CONFIG_ERROR_MEMORY;
	}
	
	/* Extract value */
	const char *value_start = equals + 1;
	char *value_buf = strdup(value_start);
	if (!value_buf) {
		free(*key);
		*key = NULL;
		return CONFIG_ERROR_MEMORY;
	}
	
	char *trimmed_value = trim_whitespace(value_buf);
	
	/* Copy the trimmed value to a new buffer */
	*value = strdup(trimmed_value);
	free(value_buf);
	if (!*value) {
		free(*key);
		*key = NULL;
		return CONFIG_ERROR_MEMORY;
	}
	
	return CONFIG_SUCCESS;
}

/**
 * @brief Add configuration entry to list
 */
static int add_config_entry(struct config_entry **entries, const char *section,
                           const char *key, const char *value)
{
	if (!entries || !key || !value) {
		return CONFIG_ERROR_INVALID;
	}
	
	struct config_entry *entry = malloc(sizeof(struct config_entry));
	if (!entry) {
		return CONFIG_ERROR_MEMORY;
	}
	
	entry->section = section ? strdup(section) : NULL;
	entry->key = strdup(key);
	entry->value = strdup(value);
	entry->next = *entries;
	
	if (!entry->key || !entry->value || (section && !entry->section)) {
		free(entry->section);
		free(entry->key);
		free(entry->value);
		free(entry);
		return CONFIG_ERROR_MEMORY;
	}
	
	*entries = entry;
	return CONFIG_SUCCESS;
}

/**
 * @brief Parse configuration file content
 */
int parse_config_content(const char *content, struct config_file *config_file)
{
	if (!content || !config_file) {
		return CONFIG_ERROR_INVALID;
	}
	
	config_file->entries = NULL;
	
	char *content_copy = strdup(content);
	if (!content_copy) {
		return CONFIG_ERROR_MEMORY;
	}
	
	char *current_section = NULL;
	char *line;
	char *saveptr;
	
	line = strtok_r(content_copy, "\n", &saveptr);
	while (line) {
		line = trim_whitespace(line);
		
		/* Skip empty lines and comments */
		if (*line == '\0' || *line == '#' || *line == ';') {
			line = strtok_r(NULL, "\n", &saveptr);
			continue;
		}
		
		/* Check for section header */
		if (*line == '[') {
			free(current_section);
			current_section = parse_section_name(line);
			line = strtok_r(NULL, "\n", &saveptr);
			continue;
		}
		
		/* Parse key-value pair */
		char *key, *value;
		if (parse_key_value(line, &key, &value) == CONFIG_SUCCESS) {
			add_config_entry(&config_file->entries, current_section, key, value);
			free(key);
			free(value);
		}
		
		line = strtok_r(NULL, "\n", &saveptr);
	}
	
	free(current_section);
	free(content_copy);
	
	return CONFIG_SUCCESS;
}

/**
 * @brief Load configuration from specific file
 */
int load_config_file(const char *file_path, struct cache_config *config)
{
	if (!file_path || !config) {
		return CONFIG_ERROR_INVALID;
	}
	
	if (!config_file_exists(file_path)) {
		return CONFIG_ERROR_NOT_FOUND;
	}
	
	FILE *file = fopen(file_path, "r");
	if (!file) {
		return CONFIG_ERROR_IO;
	}
	
	/* Get file size */
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	if (file_size <= 0) {
		fclose(file);
		return CONFIG_ERROR_IO;
	}
	
	/* Read file content */
	char *content = malloc(file_size + 1);
	if (!content) {
		fclose(file);
		return CONFIG_ERROR_MEMORY;
	}
	
	size_t read_size = fread(content, 1, file_size, file);
	content[read_size] = '\0';
	fclose(file);
	
	/* Parse content */
	struct config_file parsed_config = {0};
	int ret = parse_config_content(content, &parsed_config);
	if (ret != CONFIG_SUCCESS) {
		free(content);
		return ret;
	}
	
	/* Apply configuration */
	ret = apply_config_entries(parsed_config.entries, config);
	
	cleanup_config_entries(parsed_config.entries);
	free(content);
	
	return ret;
}

/**
 * @brief Apply configuration entries to cache config
 */
int apply_config_entries(struct config_entry *entries, struct cache_config *config)
{
	if (!config) {
		return CONFIG_ERROR_INVALID;
	}
	
	struct config_entry *entry = entries;
	while (entry) {
		/* Apply cache section settings */
		if (!entry->section || strcmp(entry->section, "cache") == 0) {
			if (strcmp(entry->key, "root") == 0) {
				if (config->cache_root) {
					free(config->cache_root);
					config->cache_root = NULL;
				}
				config->cache_root = strdup(entry->value);
				if (!config->cache_root) {
					return CONFIG_ERROR_MEMORY;
				}
			} else if (strcmp(entry->key, "checkout_root") == 0) {
				if (config->checkout_root) {
					free(config->checkout_root);
					config->checkout_root = NULL;
				}
				config->checkout_root = strdup(entry->value);
				if (!config->checkout_root) {
					return CONFIG_ERROR_MEMORY;
				}
			} else if (strcmp(entry->key, "verbose") == 0) {
				config->verbose = (strcmp(entry->value, "true") == 0 || 
				                  strcmp(entry->value, "1") == 0);
			} else if (strcmp(entry->key, "force") == 0) {
				config->force = (strcmp(entry->value, "true") == 0 || 
				                strcmp(entry->value, "1") == 0);
			}
		}
		
		/* Apply clone section settings */
		else if (strcmp(entry->section, "clone") == 0) {
			if (strcmp(entry->key, "strategy") == 0) {
				if (strcmp(entry->value, "full") == 0) {
					config->default_strategy = CLONE_STRATEGY_FULL;
				} else if (strcmp(entry->value, "shallow") == 0) {
					config->default_strategy = CLONE_STRATEGY_SHALLOW;
				} else if (strcmp(entry->value, "treeless") == 0) {
					config->default_strategy = CLONE_STRATEGY_TREELESS;
				} else if (strcmp(entry->value, "blobless") == 0) {
					config->default_strategy = CLONE_STRATEGY_BLOBLESS;
				} else if (strcmp(entry->value, "auto") == 0) {
					config->default_strategy = CLONE_STRATEGY_AUTO;
				}
			} else if (strcmp(entry->key, "recursive_submodules") == 0) {
				config->recursive_submodules = (strcmp(entry->value, "true") == 0 || 
				                               strcmp(entry->value, "1") == 0);
			}
		}
		
		/* Apply GitHub section settings */
		else if (strcmp(entry->section, "github") == 0) {
			if (strcmp(entry->key, "token") == 0) {
				if (config->github_token) {
					free(config->github_token);
					config->github_token = NULL;
				}
				config->github_token = strdup(entry->value);
				if (!config->github_token) {
					return CONFIG_ERROR_MEMORY;
				}
			}
		}
		
		entry = entry->next;
	}
	
	return CONFIG_SUCCESS;
}

/**
 * @brief Load configuration from all available sources
 */
int load_configuration(struct cache_config *config)
{
	if (!config) {
		return CONFIG_ERROR_INVALID;
	}
	
	int loaded_count = 0;
	
	/* Load system configuration */
	if (config_file_exists(CONFIG_SYSTEM_PATH)) {
		if (load_config_file(CONFIG_SYSTEM_PATH, config) == CONFIG_SUCCESS) {
			loaded_count++;
		}
	}
	
	/* Load user configuration */
	char user_config_path[PATH_MAX];
	if (get_user_config_path(user_config_path, sizeof(user_config_path)) == CONFIG_SUCCESS) {
		if (config_file_exists(user_config_path)) {
			if (load_config_file(user_config_path, config) == CONFIG_SUCCESS) {
				loaded_count++;
			}
		}
	}
	
	/* Load local configuration */
	char local_config_path[PATH_MAX];
	if (get_local_config_path(local_config_path, sizeof(local_config_path)) == CONFIG_SUCCESS) {
		if (config_file_exists(local_config_path)) {
			if (load_config_file(local_config_path, config) == CONFIG_SUCCESS) {
				loaded_count++;
			}
		}
	}
	
	/* Load from environment variable */
	const char *env_config = getenv(CONFIG_ENV_VAR);
	if (env_config && config_file_exists(env_config)) {
		if (load_config_file(env_config, config) == CONFIG_SUCCESS) {
			loaded_count++;
		}
	}
	
	return loaded_count > 0 ? CONFIG_SUCCESS : CONFIG_ERROR_NOT_FOUND;
}

/**
 * @brief Create default configuration file
 */
int create_default_config(const char *file_path)
{
	if (!file_path) {
		return CONFIG_ERROR_INVALID;
	}
	
	FILE *file = fopen(file_path, "w");
	if (!file) {
		return CONFIG_ERROR_IO;
	}
	
	fprintf(file, "# Git Cache Configuration File\n");
	fprintf(file, "# This file configures the behavior of git-cache\n");
	fprintf(file, "\n");
	
	fprintf(file, "[cache]\n");
	fprintf(file, "# Root directory for cache storage\n");
	fprintf(file, "# root = ~/.cache/git\n");
	fprintf(file, "\n");
	fprintf(file, "# Root directory for checkouts\n");
	fprintf(file, "# checkout_root = ~/github\n");
	fprintf(file, "\n");
	fprintf(file, "# Enable verbose output by default\n");
	fprintf(file, "# verbose = false\n");
	fprintf(file, "\n");
	
	fprintf(file, "[clone]\n");
	fprintf(file, "# Default clone strategy: full, shallow, treeless, blobless, auto\n");
	fprintf(file, "strategy = auto\n");
	fprintf(file, "\n");
	fprintf(file, "# Handle submodules recursively by default\n");
	fprintf(file, "recursive_submodules = true\n");
	fprintf(file, "\n");
	
	fprintf(file, "[github]\n");
	fprintf(file, "# GitHub personal access token for API operations\n");
	fprintf(file, "# token = your_github_token_here\n");
	fprintf(file, "\n");
	
	fclose(file);
	return CONFIG_SUCCESS;
}

/**
 * @brief Save current configuration to file
 */
int save_config_file(const char *file_path, const struct cache_config *config)
{
	if (!file_path || !config) {
		return CONFIG_ERROR_INVALID;
	}
	
	FILE *file = fopen(file_path, "w");
	if (!file) {
		return CONFIG_ERROR_IO;
	}
	
	fprintf(file, "# Git Cache Configuration File\n");
	fprintf(file, "# Generated by git-cache\n");
	fprintf(file, "\n");
	
	fprintf(file, "[cache]\n");
	if (config->cache_root) {
		fprintf(file, "root = %s\n", config->cache_root);
	}
	if (config->checkout_root) {
		fprintf(file, "checkout_root = %s\n", config->checkout_root);
	}
	fprintf(file, "verbose = %s\n", config->verbose ? "true" : "false");
	fprintf(file, "force = %s\n", config->force ? "true" : "false");
	fprintf(file, "\n");
	
	fprintf(file, "[clone]\n");
	const char *strategy_name = "full";
	switch (config->default_strategy) {
		case CLONE_STRATEGY_SHALLOW: strategy_name = "shallow"; break;
		case CLONE_STRATEGY_TREELESS: strategy_name = "treeless"; break;
		case CLONE_STRATEGY_BLOBLESS: strategy_name = "blobless"; break;
		case CLONE_STRATEGY_AUTO: strategy_name = "auto"; break;
		case CLONE_STRATEGY_FULL:
		default: strategy_name = "full"; break;
	}
	fprintf(file, "strategy = %s\n", strategy_name);
	fprintf(file, "recursive_submodules = %s\n", config->recursive_submodules ? "true" : "false");
	fprintf(file, "\n");
	
	fprintf(file, "[github]\n");
	if (config->github_token) {
		fprintf(file, "token = %s\n", config->github_token);
	}
	fprintf(file, "\n");
	
	fclose(file);
	return CONFIG_SUCCESS;
}

/**
 * @brief Print current configuration values
 */
void print_configuration(const struct cache_config *config)
{
	if (!config) {
		return;
	}
	
	printf("Git Cache Configuration:\n");
	printf("========================\n");
	printf("Cache root:           %s\n", config->cache_root ? config->cache_root : "(default)");
	printf("Checkout root:        %s\n", config->checkout_root ? config->checkout_root : "(default)");
	printf("GitHub token:         %s\n", config->github_token ? "***set***" : "(not set)");
	printf("Default strategy:     ");
	switch (config->default_strategy) {
		case CLONE_STRATEGY_SHALLOW: printf("shallow\n"); break;
		case CLONE_STRATEGY_TREELESS: printf("treeless\n"); break;
		case CLONE_STRATEGY_BLOBLESS: printf("blobless\n"); break;
		case CLONE_STRATEGY_AUTO: printf("auto\n"); break;
		case CLONE_STRATEGY_FULL:
		default: printf("full\n"); break;
	}
	printf("Verbose:              %s\n", config->verbose ? "true" : "false");
	printf("Force:                %s\n", config->force ? "true" : "false");
	printf("Recursive submodules: %s\n", config->recursive_submodules ? "true" : "false");
}

/**
 * @brief Validate configuration values
 */
int validate_configuration(const struct cache_config *config)
{
	if (!config) {
		return CONFIG_ERROR_INVALID;
	}
	
	/* Check if cache root exists or can be created */
	if (config->cache_root) {
		struct stat st;
		if (stat(config->cache_root, &st) != 0) {
			/* Try to create directory */
			char mkdir_cmd[4096];
			snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", config->cache_root);
			if (system(mkdir_cmd) != 0) {
				return CONFIG_ERROR_INVALID;
			}
		}
	}
	
	/* Check if checkout root exists or can be created */
	if (config->checkout_root) {
		struct stat st;
		if (stat(config->checkout_root, &st) != 0) {
			/* Try to create directory */
			char mkdir_cmd[4096];
			snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", config->checkout_root);
			if (system(mkdir_cmd) != 0) {
				return CONFIG_ERROR_INVALID;
			}
		}
	}
	
	return CONFIG_SUCCESS;
}

/**
 * @brief Clean up configuration entries list
 */
void cleanup_config_entries(struct config_entry *entries)
{
	while (entries) {
		struct config_entry *next = entries->next;
		free(entries->section);
		free(entries->key);
		free(entries->value);
		free(entries);
		entries = next;
	}
}

/**
 * @brief Clean up configuration file structure
 */
void cleanup_config_file(struct config_file *config_file)
{
	if (!config_file) {
		return;
	}
	
	free(config_file->path);
	cleanup_config_entries(config_file->entries);
	memset(config_file, 0, sizeof(*config_file));
}

/**
 * @brief Get configuration error message
 */
const char* config_get_error_string(int error_code)
{
	switch (error_code) {
		case CONFIG_SUCCESS:
			return "Success";
		case CONFIG_ERROR_NOT_FOUND:
			return "Configuration file not found";
		case CONFIG_ERROR_PARSE:
			return "Configuration file parse error";
		case CONFIG_ERROR_IO:
			return "Configuration file I/O error";
		case CONFIG_ERROR_MEMORY:
			return "Memory allocation error";
		case CONFIG_ERROR_INVALID:
			return "Invalid configuration";
		default:
			return "Unknown error";
	}
}

/**
 * @brief Find configuration entry by section and key
 */
static struct config_entry* find_config_entry(const char *section, const char *key)
{
	struct config_entry *entry = global_config.entries;
	while (entry) {
		if ((!section && !entry->section) || 
		    (section && entry->section && strcmp(section, entry->section) == 0)) {
			if (strcmp(key, entry->key) == 0) {
				return entry;
			}
		}
		entry = entry->next;
	}
	return NULL;
}

/**
 * @brief Get configuration value as string
 */
const char* get_config_string(const char *section, const char *key, const char *default_value)
{
	struct config_entry *entry = find_config_entry(section, key);
	return entry ? entry->value : default_value;
}

/**
 * @brief Get configuration value as integer
 */
int get_config_int(const char *section, const char *key, int default_value)
{
	struct config_entry *entry = find_config_entry(section, key);
	if (!entry) {
		return default_value;
	}
	
	char *endptr;
	long value = strtol(entry->value, &endptr, 10);
	if (endptr == entry->value || *endptr != '\0') {
		return default_value;
	}
	
	return (int)value;
}

/**
 * @brief Get configuration value as boolean
 */
int get_config_bool(const char *section, const char *key, int default_value)
{
	struct config_entry *entry = find_config_entry(section, key);
	if (!entry) {
		return default_value;
	}
	
	if (strcmp(entry->value, "true") == 0 || strcmp(entry->value, "1") == 0 ||
	    strcmp(entry->value, "yes") == 0 || strcmp(entry->value, "on") == 0) {
		return 1;
	} else if (strcmp(entry->value, "false") == 0 || strcmp(entry->value, "0") == 0 ||
	           strcmp(entry->value, "no") == 0 || strcmp(entry->value, "off") == 0) {
		return 0;
	}
	
	return default_value;
}

/**
 * @brief Set configuration value
 */
int set_config_value(const char *section, const char *key, const char *value)
{
	if (!key || !value) {
		return CONFIG_ERROR_INVALID;
	}
	
	/* Find existing entry */
	struct config_entry *entry = find_config_entry(section, key);
	if (entry) {
		/* Update existing entry */
		char *new_value = strdup(value);
		if (!new_value) {
			return CONFIG_ERROR_MEMORY;
		}
		free(entry->value);
		entry->value = new_value;
		return CONFIG_SUCCESS;
	}
	
	/* Add new entry */
	return add_config_entry(&global_config.entries, section, key, value);
}