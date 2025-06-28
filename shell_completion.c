/**
 * @file shell_completion.c
 * @brief Implementation of shell completion support for git-cache
 * 
 * Provides tab completion functionality for bash, zsh, and fish shells
 * including command completion, option completion, and URL completion.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#include "git-cache.h"
#include "shell_completion.h"

/**
 * @brief Bash completion script template
 */
static const char bash_completion_script[] = 
"#!/bin/bash\n"
"# Git-cache bash completion\n"
"\n"
"_git_cache_completion() {\n"
"    local cur prev opts commands\n"
"    COMPREPLY=()\n"
"    cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
"    prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n"
"\n"
"    commands=\"clone status clean sync list verify repair config mirror completion\"\n"
"    opts=\"-h --help -v --verbose -V --version -f --force --strategy --depth --org --private --recursive\"\n"
"\n"
"    if [[ ${COMP_CWORD} == 1 ]]; then\n"
"        COMPREPLY=($(compgen -W \"${commands}\" -- ${cur}))\n"
"        return 0\n"
"    fi\n"
"\n"
"    case \"${prev}\" in\n"
"        --strategy)\n"
"            COMPREPLY=($(compgen -W \"full shallow treeless blobless auto\" -- ${cur}))\n"
"            return 0\n"
"            ;;\n"
"        --depth)\n"
"            COMPREPLY=($(compgen -W \"1 5 10 50\" -- ${cur}))\n"
"            return 0\n"
"            ;;\n"
"        clone)\n"
"            # Complete git URLs\n"
"            if [[ ${cur} == git@* ]] || [[ ${cur} == https://* ]] || [[ ${cur} == http://* ]]; then\n"
"                return 0\n"
"            fi\n"
"            ;;\n"
"        config)\n"
"            COMPREPLY=($(compgen -W \"init show set get\" -- ${cur}))\n"
"            return 0\n"
"            ;;\n"
"        mirror)\n"
"            COMPREPLY=($(compgen -W \"add remove list sync\" -- ${cur}))\n"
"            return 0\n"
"            ;;\n"
"        completion)\n"
"            COMPREPLY=($(compgen -W \"status install uninstall generate\" -- ${cur}))\n"
"            return 0\n"
"            ;;\n"
"    esac\n"
"\n"
"    COMPREPLY=($(compgen -W \"${opts}\" -- ${cur}))\n"
"}\n"
"\n"
"complete -F _git_cache_completion git-cache\n";

/**
 * @brief Zsh completion script template
 */
static const char zsh_completion_script[] = 
"#compdef git-cache\n"
"# Git-cache zsh completion\n"
"\n"
"_git_cache() {\n"
"    local context state line\n"
"    typeset -A opt_args\n"
"\n"
"    _arguments -C \\\n"
"        '1: :_git_cache_commands' \\\n"
"        '*:: :->args' \\\n"
"        '(-h --help)'{-h,--help}'[Show help message]' \\\n"
"        '(-v --verbose)'{-v,--verbose}'[Enable verbose output]' \\\n"
"        '(-V --version)'{-V,--version}'[Show version information]' \\\n"
"        '(-f --force)'{-f,--force}'[Force operation]' \\\n"
"        '--strategy[Clone strategy]:strategy:(full shallow treeless blobless auto)' \\\n"
"        '--depth[Depth for shallow clones]:depth:(1 5 10 50)' \\\n"
"        '--org[Organization for forks]:organization:' \\\n"
"        '--private[Make forked repositories private]' \\\n"
"        '--recursive[Handle submodules recursively]'\n"
"\n"
"    case $state in\n"
"        args)\n"
"            case $words[1] in\n"
"                clone)\n"
"                    _arguments '*:repository URL:'\n"
"                    ;;\n"
"                config)\n"
"                    _arguments '1:config command:(init show set get)'\n"
"                    ;;\n"
"                mirror)\n"
"                    _arguments '1:mirror command:(add remove list sync)'\n"
"                    ;;\n"
"                completion)\n"
"                    _arguments '1:completion command:(status install uninstall generate)'\n"
"                    ;;\n"
"                verify)\n"
"                    _arguments '*:repository URL:'\n"
"                    ;;\n"
"            esac\n"
"            ;;\n"
"    esac\n"
"}\n"
"\n"
"_git_cache_commands() {\n"
"    local commands\n"
"    commands=(\n"
"        'clone:Clone repository with caching'\n"
"        'status:Show cache status'\n"
"        'clean:Clean cache'\n"
"        'sync:Synchronize cache with remotes'\n"
"        'list:List cached repositories'\n"
"        'verify:Verify cache integrity and repair if needed'\n"
"        'repair:Repair outdated checkouts'\n"
"        'config:Show or modify configuration'\n"
"        'mirror:Manage remote mirrors'\n"
"        'completion:Manage shell completion'\n"
"    )\n"
"    _describe 'commands' commands\n"
"}\n"
"\n"
"_git_cache \"$@\"\n";

/**
 * @brief Fish completion script template
 */
static const char fish_completion_script[] = 
"# Git-cache fish completion\n"
"\n"
"# Commands\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'clone' -d 'Clone repository with caching'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'status' -d 'Show cache status'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'clean' -d 'Clean cache'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'sync' -d 'Synchronize cache with remotes'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'list' -d 'List cached repositories'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'verify' -d 'Verify cache integrity'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'repair' -d 'Repair outdated checkouts'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'config' -d 'Show or modify configuration'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'mirror' -d 'Manage remote mirrors'\n"
"complete -c git-cache -n '__fish_use_subcommand' -a 'completion' -d 'Manage shell completion'\n"
"\n"
"# Global options\n"
"complete -c git-cache -s h -l help -d 'Show help message'\n"
"complete -c git-cache -s v -l verbose -d 'Enable verbose output'\n"
"complete -c git-cache -s V -l version -d 'Show version information'\n"
"complete -c git-cache -s f -l force -d 'Force operation'\n"
"complete -c git-cache -l recursive -d 'Handle submodules recursively'\n"
"complete -c git-cache -l private -d 'Make forked repositories private'\n"
"\n"
"# Strategy options\n"
"complete -c git-cache -l strategy -d 'Clone strategy' -xa 'full shallow treeless blobless auto'\n"
"\n"
"# Depth options\n"
"complete -c git-cache -l depth -d 'Depth for shallow clones' -xa '1 5 10 50'\n"
"\n"
"# Organization option\n"
"complete -c git-cache -l org -d 'Organization for forks'\n"
"\n"
"# Config subcommands\n"
"complete -c git-cache -n '__fish_seen_subcommand_from config' -xa 'init show set get'\n"
"\n"
"# Mirror subcommands\n"
"complete -c git-cache -n '__fish_seen_subcommand_from mirror' -xa 'add remove list sync'\n"
"\n"
"# Completion subcommands\n"
"complete -c git-cache -n '__fish_seen_subcommand_from completion' -xa 'status install uninstall generate'\n";

/**
 * @brief Auto-detect current shell type
 */
enum shell_type detect_shell_type(void)
{
	const char *shell = getenv("SHELL");
	if (!shell) {
		return SHELL_TYPE_UNKNOWN;
	}
	
	if (strstr(shell, "bash")) {
		return SHELL_TYPE_BASH;
	} else if (strstr(shell, "zsh")) {
		return SHELL_TYPE_ZSH;
	} else if (strstr(shell, "fish")) {
		return SHELL_TYPE_FISH;
	}
	
	return SHELL_TYPE_UNKNOWN;
}

/**
 * @brief Get shell type name as string
 */
const char* get_shell_name(enum shell_type shell_type)
{
	switch (shell_type) {
		case SHELL_TYPE_BASH:
			return "bash";
		case SHELL_TYPE_ZSH:
			return "zsh";
		case SHELL_TYPE_FISH:
			return "fish";
		default:
			return "unknown";
	}
}

/**
 * @brief Parse shell type from string
 */
enum shell_type parse_shell_type(const char *shell_name)
{
	if (!shell_name) {
		return SHELL_TYPE_UNKNOWN;
	}
	
	if (strcmp(shell_name, "bash") == 0) {
		return SHELL_TYPE_BASH;
	} else if (strcmp(shell_name, "zsh") == 0) {
		return SHELL_TYPE_ZSH;
	} else if (strcmp(shell_name, "fish") == 0) {
		return SHELL_TYPE_FISH;
	}
	
	return SHELL_TYPE_UNKNOWN;
}

/**
 * @brief Get completion installation path for shell
 */
int get_completion_path(enum shell_type shell_type, char *path, size_t path_size)
{
	if (!path || path_size == 0) {
		return -1;
	}
	
	const char *home = getenv("HOME");
	if (!home) {
		return -1;
	}
	
	switch (shell_type) {
		case SHELL_TYPE_BASH:
			snprintf(path, path_size, "%s/.bash_completion.d/git-cache", home);
			break;
		case SHELL_TYPE_ZSH:
			snprintf(path, path_size, "%s/.zsh/completions/_git-cache", home);
			break;
		case SHELL_TYPE_FISH:
			snprintf(path, path_size, "%s/.config/fish/completions/git-cache.fish", home);
			break;
		default:
			return -1;
	}
	
	return 0;
}

/**
 * @brief Generate shell completion script for specified shell
 */
int generate_completion_script(enum shell_type shell_type, const char *output_file)
{
	const char *script_content = NULL;
	FILE *output = stdout;
	
	switch (shell_type) {
		case SHELL_TYPE_BASH:
			script_content = bash_completion_script;
			break;
		case SHELL_TYPE_ZSH:
			script_content = zsh_completion_script;
			break;
		case SHELL_TYPE_FISH:
			script_content = fish_completion_script;
			break;
		default:
			fprintf(stderr, "error: unsupported shell type\n");
			return -1;
	}
	
	if (output_file) {
		output = fopen(output_file, "w");
		if (!output) {
			fprintf(stderr, "error: failed to open output file '%s': %s\n", 
			        output_file, strerror(errno));
			return -1;
		}
	}
	
	if (fputs(script_content, output) == EOF) {
		if (output != stdout) {
			fclose(output);
		}
		return -1;
	}
	
	if (output != stdout) {
		fclose(output);
	}
	
	return 0;
}

/**
 * @brief Create directory recursively
 */
static int create_directory_recursive(const char *path)
{
	char tmp[PATH_MAX];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp);
	if (tmp[len - 1] == '/') {
		tmp[len - 1] = 0;
	}
	
	for (p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = 0;
			if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
				return -1;
			}
			*p = '/';
		}
	}
	
	if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
		return -1;
	}
	
	return 0;
}

/**
 * @brief Install shell completion for current user
 */
int install_shell_completion(enum shell_type shell_type)
{
	char completion_path[PATH_MAX];
	char dir_path[PATH_MAX];
	char *last_slash;
	
	if (shell_type == SHELL_TYPE_UNKNOWN) {
		shell_type = detect_shell_type();
		if (shell_type == SHELL_TYPE_UNKNOWN) {
			fprintf(stderr, "error: could not detect shell type\n");
			return -1;
		}
	}
	
	if (get_completion_path(shell_type, completion_path, sizeof(completion_path)) != 0) {
		fprintf(stderr, "error: failed to get completion path\n");
		return -1;
	}
	
	/* Create directory if it doesn't exist */
	strcpy(dir_path, completion_path);
	last_slash = strrchr(dir_path, '/');
	if (last_slash) {
		*last_slash = '\0';
		if (create_directory_recursive(dir_path) != 0) {
			fprintf(stderr, "error: failed to create directory '%s': %s\n", 
			        dir_path, strerror(errno));
			return -1;
		}
	}
	
	/* Generate completion script to file */
	if (generate_completion_script(shell_type, completion_path) != 0) {
		fprintf(stderr, "error: failed to generate completion script\n");
		return -1;
	}
	
	printf("Shell completion installed for %s at: %s\n", 
	       get_shell_name(shell_type), completion_path);
	
	/* Provide instructions for enabling completion */
	switch (shell_type) {
		case SHELL_TYPE_BASH:
			printf("\nTo enable completion in current session, run:\n");
			printf("  source %s\n", completion_path);
			printf("\nTo enable permanently, add this to your ~/.bashrc:\n");
			printf("  source %s\n", completion_path);
			break;
		case SHELL_TYPE_ZSH:
			printf("\nTo enable completion, add this to your ~/.zshrc:\n");
			printf("  fpath=(~/.zsh/completions $fpath)\n");
			printf("  autoload -U compinit && compinit\n");
			break;
		case SHELL_TYPE_FISH:
			printf("\nCompletion will be automatically available in new fish sessions.\n");
			break;
		default:
			break;
	}
	
	return 0;
}

/**
 * @brief Check if shell completion is installed
 */
int is_completion_installed(enum shell_type shell_type)
{
	char completion_path[PATH_MAX];
	struct stat st;
	
	if (get_completion_path(shell_type, completion_path, sizeof(completion_path)) != 0) {
		return -1;
	}
	
	return (stat(completion_path, &st) == 0) ? 1 : 0;
}

/**
 * @brief Uninstall shell completion for current user
 */
int uninstall_shell_completion(enum shell_type shell_type)
{
	char completion_path[PATH_MAX];
	
	if (shell_type == SHELL_TYPE_UNKNOWN) {
		shell_type = detect_shell_type();
		if (shell_type == SHELL_TYPE_UNKNOWN) {
			fprintf(stderr, "error: could not detect shell type\n");
			return -1;
		}
	}
	
	if (get_completion_path(shell_type, completion_path, sizeof(completion_path)) != 0) {
		fprintf(stderr, "error: failed to get completion path\n");
		return -1;
	}
	
	if (unlink(completion_path) != 0) {
		if (errno == ENOENT) {
			printf("Shell completion for %s is not installed\n", get_shell_name(shell_type));
			return 0;
		} else {
			fprintf(stderr, "error: failed to remove completion file '%s': %s\n", 
			        completion_path, strerror(errno));
			return -1;
		}
	}
	
	printf("Shell completion uninstalled for %s\n", get_shell_name(shell_type));
	return 0;
}

/**
 * @brief Show completion status for all supported shells
 */
int show_completion_status(void)
{
	printf("Shell completion status:\n");
	
	enum shell_type shells[] = {SHELL_TYPE_BASH, SHELL_TYPE_ZSH, SHELL_TYPE_FISH};
	int num_shells = sizeof(shells) / sizeof(shells[0]);
	
	for (int i = 0; i < num_shells; i++) {
		int installed = is_completion_installed(shells[i]);
		const char *status = (installed == 1) ? "installed" : "not installed";
		
		printf("  %-6s: %s", get_shell_name(shells[i]), status);
		
		if (installed == 1) {
			char completion_path[PATH_MAX];
			if (get_completion_path(shells[i], completion_path, sizeof(completion_path)) == 0) {
				printf(" (%s)", completion_path);
			}
		}
		printf("\n");
	}
	
	enum shell_type current_shell = detect_shell_type();
	if (current_shell != SHELL_TYPE_UNKNOWN) {
		printf("\nCurrent shell: %s\n", get_shell_name(current_shell));
	}
	
	return 0;
}

/**
 * @brief Handle completion command from command line
 */
int handle_completion_command(enum completion_mode mode, enum shell_type shell_type, 
	                         const char *output_file)
{
	switch (mode) {
		case COMPLETION_MODE_GENERATE:
			if (shell_type == SHELL_TYPE_UNKNOWN) {
				shell_type = detect_shell_type();
				if (shell_type == SHELL_TYPE_UNKNOWN) {
					fprintf(stderr, "error: could not detect shell type, specify with --shell\n");
					return -1;
				}
			}
			return generate_completion_script(shell_type, output_file);
			
		case COMPLETION_MODE_INSTALL:
			return install_shell_completion(shell_type);
			
		case COMPLETION_MODE_UNINSTALL:
			return uninstall_shell_completion(shell_type);
			
		case COMPLETION_MODE_STATUS:
			return show_completion_status();
			
		default:
			fprintf(stderr, "error: unknown completion mode\n");
			return -1;
	}
}