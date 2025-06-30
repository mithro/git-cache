/**
 * @file fork_config.c
 * @brief Implementation of fork repository configuration and management
 * 
 * Provides functionality for configuring how git-cache handles repository
 * forking, including organization settings and synchronization.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#include "git-cache.h"
#include "fork_config.h"
#include "github_api.h"
#include "config_file.h"

/**
 * @brief Get default fork configuration
 */
void get_default_fork_config(struct fork_config *config)
{
	if (!config) {
		return;
	}
	
	memset(config, 0, sizeof(*config));
	config->auto_fork = 1;
	config->fork_private_as_private = 1;
	config->fork_public_as_private = 0;
	config->sync_with_upstream = 1;
	config->sync_interval_hours = 24;
	config->delete_branch_on_merge = 1;
	config->allow_force_push = 0;
	config->branch_prefix = strdup("git-cache/");
}

/**
 * @brief Load fork configuration from files and environment
 */
int load_fork_config(struct fork_config *config)
{
	if (!config) {
		return -1;
	}
	
	/* Start with defaults */
	get_default_fork_config(config);
	
	/* Load from configuration files using config_file module */
	const char *org = get_config_string("fork", "organization", NULL);
	if (org) {
		free(config->default_organization);
		config->default_organization = strdup(org);
	}
	
	config->auto_fork = get_config_bool("fork", "auto_fork", config->auto_fork);
	config->fork_private_as_private = get_config_bool("fork", "private_as_private", 
	                                                  config->fork_private_as_private);
	config->fork_public_as_private = get_config_bool("fork", "public_as_private",
	                                                config->fork_public_as_private);
	config->sync_with_upstream = get_config_bool("fork", "sync_upstream", 
	                                            config->sync_with_upstream);
	config->sync_interval_hours = get_config_int("fork", "sync_interval_hours",
	                                           config->sync_interval_hours);
	config->delete_branch_on_merge = get_config_bool("fork", "delete_merged_branches",
	                                                config->delete_branch_on_merge);
	config->allow_force_push = get_config_bool("fork", "allow_force_push",
	                                          config->allow_force_push);
	
	const char *prefix = get_config_string("fork", "branch_prefix", config->branch_prefix);
	if (prefix && strcmp(prefix, config->branch_prefix) != 0) {
		free(config->branch_prefix);
		config->branch_prefix = strdup(prefix);
	}
	
	/* Override with environment variables */
	const char *env_org = getenv("GIT_CACHE_FORK_ORG");
	if (env_org) {
		free(config->default_organization);
		config->default_organization = strdup(env_org);
	}
	
	const char *env_auto = getenv("GIT_CACHE_AUTO_FORK");
	if (env_auto) {
		config->auto_fork = (strcmp(env_auto, "1") == 0 || 
		                    strcmp(env_auto, "true") == 0);
	}
	
	const char *env_private = getenv("GIT_CACHE_FORK_PRIVATE");
	if (env_private) {
		config->fork_public_as_private = (strcmp(env_private, "1") == 0 ||
		                                 strcmp(env_private, "true") == 0);
	}
	
	return 0;
}

/**
 * @brief Save fork configuration to file
 */
int save_fork_config(const struct fork_config *config)
{
	if (!config) {
		return -1;
	}
	
	/* Save to configuration file */
	if (config->default_organization) {
		set_config_value("fork", "organization", config->default_organization);
	}
	
	set_config_value("fork", "auto_fork", config->auto_fork ? "true" : "false");
	set_config_value("fork", "private_as_private", 
	                config->fork_private_as_private ? "true" : "false");
	set_config_value("fork", "public_as_private",
	                config->fork_public_as_private ? "true" : "false");
	set_config_value("fork", "sync_upstream",
	                config->sync_with_upstream ? "true" : "false");
	
	char interval_str[32];
	snprintf(interval_str, sizeof(interval_str), "%d", config->sync_interval_hours);
	set_config_value("fork", "sync_interval_hours", interval_str);
	
	set_config_value("fork", "delete_merged_branches",
	                config->delete_branch_on_merge ? "true" : "false");
	set_config_value("fork", "allow_force_push",
	                config->allow_force_push ? "true" : "false");
	
	if (config->branch_prefix) {
		set_config_value("fork", "branch_prefix", config->branch_prefix);
	}
	
	return 0;
}

/**
 * @brief Check if repository needs forking
 */
int needs_fork(const struct repo_info *repo, const struct fork_config *config)
{
	if (!repo || !config) {
		return -1;
	}
	
	/* Already have a fork */
	if (repo->fork_url) {
		return 0;
	}
	
	/* Auto-fork disabled */
	if (!config->auto_fork) {
		return 0;
	}
	
	/* Check if we have push access (would need actual API call) */
	/* For now, assume we need fork if is_fork_needed is set */
	return repo->is_fork_needed ? 1 : 0;
}

/**
 * @brief Create fork with specified settings
 */
int create_fork_with_config(struct github_client *client, const char *owner,
	                       const char *name, const struct fork_config *config,
	                       struct fork_result *result)
{
	if (!client || !owner || !name || !config || !result) {
		return -1;
	}
	
	/* Initialize result */
	memset(result, 0, sizeof(*result));
	
	/* Fork the repository */
	struct github_repo *forked_repo = NULL;
	int ret = github_fork_repo(client, owner, name, config->default_organization, &forked_repo);
	
	if (ret != 0) {
		result->success = 0;
		result->error_message = strdup("Failed to create fork");
		return ret;
	}
	
	if (forked_repo) {
		result->success = 1;
		result->fork_url = strdup(forked_repo->clone_url);
		result->already_exists = 0;
		
		/* Apply privacy settings if needed */
		if ((forked_repo->is_private && !config->fork_private_as_private) ||
		    (!forked_repo->is_private && config->fork_public_as_private)) {
			int make_private = config->fork_public_as_private || 
			                  (forked_repo->is_private && config->fork_private_as_private);
			github_set_repo_private(client, forked_repo->owner, forked_repo->name, make_private);
		}
		
		github_repo_destroy(forked_repo);
	}
	
	return 0;
}

/**
 * @brief Configure fork remote in local repository
 */
int configure_fork_remotes(const char *repo_path, const char *fork_url,
	                      const char *upstream_url)
{
	if (!repo_path || !fork_url || !upstream_url) {
		return -1;
	}
	
	/* Set origin to fork URL */
	char set_origin_cmd[8192];
	snprintf(set_origin_cmd, sizeof(set_origin_cmd),
	         "cd \"%s\" && git remote set-url origin \"%s\" 2>/dev/null || "
	         "git remote add origin \"%s\"",
	         repo_path, fork_url, fork_url);
	
	if (system(set_origin_cmd) != 0) {
		return -1;
	}
	
	/* Add upstream remote */
	char add_upstream_cmd[8192];
	snprintf(add_upstream_cmd, sizeof(add_upstream_cmd),
	         "cd \"%s\" && git remote add upstream \"%s\" 2>/dev/null || "
	         "git remote set-url upstream \"%s\"",
	         repo_path, upstream_url, upstream_url);
	
	if (system(add_upstream_cmd) != 0) {
		return -1;
	}
	
	return 0;
}

/**
 * @brief Synchronize fork with upstream
 */
int sync_fork_with_upstream(const struct repo_info *repo, const char *branch, int force)
{
	if (!repo || !repo->modifiable_path) {
		return -1;
	}
	
	/* Fetch from upstream */
	char fetch_cmd[4096];
	snprintf(fetch_cmd, sizeof(fetch_cmd),
	         "cd \"%s\" && git fetch upstream 2>&1",
	         repo->modifiable_path);
	
	if (system(fetch_cmd) != 0) {
		return -1;
	}
	
	/* Merge or rebase based on configuration */
	char sync_cmd[8192];
	if (branch) {
		snprintf(sync_cmd, sizeof(sync_cmd),
		         "cd \"%s\" && git checkout %s && git merge upstream/%s %s",
		         repo->modifiable_path, branch, branch,
		         force ? "--allow-unrelated-histories" : "");
	} else {
		/* Sync default branch */
		snprintf(sync_cmd, sizeof(sync_cmd),
		         "cd \"%s\" && git checkout $(git symbolic-ref --short HEAD) && "
		         "git merge upstream/$(git symbolic-ref --short HEAD) %s",
		         repo->modifiable_path,
		         force ? "--allow-unrelated-histories" : "");
	}
	
	return system(sync_cmd) == 0 ? 0 : -1;
}

/**
 * @brief Get fork synchronization status
 */
int get_fork_sync_status(const struct repo_info *repo, struct fork_sync_status *status)
{
	if (!repo || !status || !repo->modifiable_path) {
		return -1;
	}
	
	memset(status, 0, sizeof(*status));
	
	/* Set URLs */
	if (repo->fork_url) {
		status->fork_url = strdup(repo->fork_url);
	}
	if (repo->original_url) {
		status->upstream_url = strdup(repo->original_url);
	}
	
	/* Get commits behind/ahead */
	char behind_cmd[4096];
	snprintf(behind_cmd, sizeof(behind_cmd),
	         "cd \"%s\" && git rev-list --count HEAD..upstream/HEAD 2>/dev/null",
	         repo->modifiable_path);
	
	FILE *pipe = popen(behind_cmd, "r");
	if (pipe) {
		if (fscanf(pipe, "%d", &status->commits_behind) != 1) {
			status->commits_behind = 0;
		}
		pclose(pipe);
	}
	
	char ahead_cmd[4096];
	snprintf(ahead_cmd, sizeof(ahead_cmd),
	         "cd \"%s\" && git rev-list --count upstream/HEAD..HEAD 2>/dev/null",
	         repo->modifiable_path);
	
	pipe = popen(ahead_cmd, "r");
	if (pipe) {
		if (fscanf(pipe, "%d", &status->commits_ahead) != 1) {
			status->commits_ahead = 0;
		}
		pclose(pipe);
	}
	
	status->last_sync = time(NULL);
	status->has_conflicts = 0; /* Would need to actually try merge to detect */
	
	return 0;
}

/**
 * @brief Check if user has permission to push to repository
 */
int can_push_to_repository(struct github_client *client, const char *owner,
	                      const char *name)
{
	if (!client || !owner || !name) {
		return -1;
	}
	
	/* Get repository information */
	struct github_repo *repo = NULL;
	int ret = github_get_repo(client, owner, name, &repo);
	
	if (ret != 0 || !repo) {
		return 0;
	}
	
	/* For now, assume we can't push to repos we don't own */
	/* Would need to check permissions via API */
	int can_push = 0;
	
	github_repo_destroy(repo);
	return can_push;
}

/**
 * @brief Set default fork organization
 */
int set_default_fork_organization(struct fork_config *config, const char *organization)
{
	if (!config) {
		return -1;
	}
	
	free(config->default_organization);
	config->default_organization = organization ? strdup(organization) : NULL;
	
	return 0;
}

/**
 * @brief Clean up fork configuration structure
 */
void cleanup_fork_config(struct fork_config *config)
{
	if (!config) {
		return;
	}
	
	free(config->default_organization);
	free(config->branch_prefix);
	memset(config, 0, sizeof(*config));
}

/**
 * @brief Clean up fork result structure
 */
void cleanup_fork_result(struct fork_result *result)
{
	if (!result) {
		return;
	}
	
	free(result->fork_url);
	free(result->error_message);
	memset(result, 0, sizeof(*result));
}

/**
 * @brief Clean up fork sync status structure
 */
void cleanup_fork_sync_status(struct fork_sync_status *status)
{
	if (!status) {
		return;
	}
	
	free(status->fork_url);
	free(status->upstream_url);
	memset(status, 0, sizeof(*status));
}