#ifndef SHELL_COMPLETION_H
#define SHELL_COMPLETION_H

/**
 * @file shell_completion.h
 * @brief Shell completion support for git-cache
 * 
 * This module provides functionality for generating shell completion scripts
 * for bash, zsh, and fish shells to enable tab completion for git-cache commands.
 */

#include "git-cache.h"

/* Forward declarations */
struct cache_config;

/**
 * @brief Shell types supported for completion
 */
enum shell_type {
	SHELL_TYPE_BASH,
	SHELL_TYPE_ZSH,
	SHELL_TYPE_FISH,
	SHELL_TYPE_UNKNOWN
};

/**
 * @brief Completion modes
 */
enum completion_mode {
	COMPLETION_MODE_INSTALL,   /**< Install completion scripts */
	COMPLETION_MODE_UNINSTALL, /**< Remove completion scripts */
	COMPLETION_MODE_GENERATE,  /**< Generate completion script to stdout */
	COMPLETION_MODE_STATUS     /**< Show completion status */
};

/**
 * @brief Generate shell completion script for specified shell
 * @param shell_type Type of shell to generate completion for
 * @param output_file File to write completion script (NULL for stdout)
 * @return 0 on success, negative error code on failure
 */
int generate_completion_script(enum shell_type shell_type, const char *output_file);

/**
 * @brief Install shell completion for current user
 * @param shell_type Type of shell (SHELL_TYPE_UNKNOWN for auto-detect)
 * @return 0 on success, negative error code on failure
 */
int install_shell_completion(enum shell_type shell_type);

/**
 * @brief Uninstall shell completion for current user
 * @param shell_type Type of shell (SHELL_TYPE_UNKNOWN for auto-detect)
 * @return 0 on success, negative error code on failure
 */
int uninstall_shell_completion(enum shell_type shell_type);

/**
 * @brief Check if shell completion is installed
 * @param shell_type Type of shell to check
 * @return 1 if installed, 0 if not, negative on error
 */
int is_completion_installed(enum shell_type shell_type);

/**
 * @brief Auto-detect current shell type
 * @return Detected shell type or SHELL_TYPE_UNKNOWN
 */
enum shell_type detect_shell_type(void);

/**
 * @brief Get completion installation path for shell
 * @param shell_type Type of shell
 * @param path Buffer to store path (should be PATH_MAX size)
 * @param path_size Size of path buffer
 * @return 0 on success, negative error code on failure
 */
int get_completion_path(enum shell_type shell_type, char *path, size_t path_size);

/**
 * @brief Show completion status for all supported shells
 * @return 0 on success, negative error code on failure
 */
int show_completion_status(void);

/**
 * @brief Get shell type name as string
 * @param shell_type Shell type
 * @return Shell name string
 */
const char* get_shell_name(enum shell_type shell_type);

/**
 * @brief Parse shell type from string
 * @param shell_name Shell name string
 * @return Shell type or SHELL_TYPE_UNKNOWN
 */
enum shell_type parse_shell_type(const char *shell_name);

/**
 * @brief Handle completion command from command line
 * @param mode Completion mode
 * @param shell_type Shell type (can be SHELL_TYPE_UNKNOWN for auto-detect)
 * @param output_file Output file for generate mode (NULL for stdout)
 * @return 0 on success, negative error code on failure
 */
int handle_completion_command(enum completion_mode mode, enum shell_type shell_type, 
	                         const char *output_file);

#endif /* SHELL_COMPLETION_H */