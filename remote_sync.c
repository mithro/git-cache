/**
 * @file remote_sync.c
 * @brief Implementation of remote mirror management and synchronization
 * 
 * Provides functionality for managing multiple remote mirrors and
 * synchronizing repositories across different remote locations.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#include "git-cache.h"
#include "remote_sync.h"
#include "cache_metadata.h"

/**
 * @brief Load synchronization configuration with defaults
 */
int load_sync_config(struct sync_config *config)
{
	if (!config) {
		return SYNC_ERROR_INVALID;
	}
	
	/* Set default values */
	config->auto_sync = 0;
	config->sync_interval_hours = 24;
	config->max_concurrent_syncs = 3;
	config->retry_count = 3;
	config->retry_delay_seconds = 30;
	config->preferred_mirror = NULL;
	config->fallback_enabled = 1;
	
	/* Load from environment variables */
	const char *auto_sync = getenv("GIT_CACHE_AUTO_SYNC");
	if (auto_sync && strcmp(auto_sync, "1") == 0) {
		config->auto_sync = 1;
	}
	
	const char *sync_interval = getenv("GIT_CACHE_SYNC_INTERVAL");
	if (sync_interval) {
		int interval = atoi(sync_interval);
		if (interval > 0) {
			config->sync_interval_hours = interval;
		}
	}
	
	const char *preferred_mirror = getenv("GIT_CACHE_PREFERRED_MIRROR");
	if (preferred_mirror) {
		config->preferred_mirror = strdup(preferred_mirror);
	}
	
	return SYNC_SUCCESS;
}

/**
 * @brief Add remote mirror to repository
 */
int add_remote_mirror(struct repo_info *repo, const char *mirror_name,
	                 const char *mirror_url, const char *mirror_type, int priority)
{
	if (!repo || !mirror_name || !mirror_url || !repo->cache_path) {
		return SYNC_ERROR_INVALID;
	}
	
	/* Add git remote to the cached repository */
	char add_remote_cmd[8192];
	snprintf(add_remote_cmd, sizeof(add_remote_cmd),
	         "cd \"%s\" && git remote add \"%s\" \"%s\" 2>/dev/null || "
	         "git remote set-url \"%s\" \"%s\"",
	         repo->cache_path, mirror_name, mirror_url, mirror_name, mirror_url);
	
	int result = system(add_remote_cmd);
	if (WEXITSTATUS(result) != 0) {
		return SYNC_ERROR_NETWORK;
	}
	
	/* Store mirror metadata */
	char metadata_file[4096];
	snprintf(metadata_file, sizeof(metadata_file), "%s/mirrors.txt", repo->cache_path);
	
	FILE *file = fopen(metadata_file, "a");
	if (file) {
		fprintf(file, "%s\t%s\t%s\t%d\t%ld\n", 
		        mirror_name, mirror_url, mirror_type ? mirror_type : "backup", 
		        priority, time(NULL));
		fclose(file);
	}
	
	return SYNC_SUCCESS;
}

/**
 * @brief Remove remote mirror from repository
 */
int remove_remote_mirror(struct repo_info *repo, const char *mirror_name)
{
	if (!repo || !mirror_name || !repo->cache_path) {
		return SYNC_ERROR_INVALID;
	}
	
	/* Remove git remote from the cached repository */
	char remove_remote_cmd[4096];
	snprintf(remove_remote_cmd, sizeof(remove_remote_cmd),
	         "cd \"%s\" && git remote remove \"%s\" 2>/dev/null",
	         repo->cache_path, mirror_name);
	
	int result = system(remove_remote_cmd);
	if (WEXITSTATUS(result) != 0) {
		return SYNC_ERROR_NOT_FOUND;
	}
	
	return SYNC_SUCCESS;
}

/**
 * @brief Synchronize repository with specific mirror
 */
int sync_with_mirror(struct repo_info *repo, const char *mirror_name, int force)
{
	if (!repo || !mirror_name || !repo->cache_path) {
		return SYNC_ERROR_INVALID;
	}
	
	/* Fetch from the specific mirror */
	char fetch_cmd[8192];
	snprintf(fetch_cmd, sizeof(fetch_cmd),
	         "cd \"%s\" && git fetch \"%s\" %s 2>&1",
	         repo->cache_path, mirror_name, force ? "--force" : "");
	
	int result = system(fetch_cmd);
	if (WEXITSTATUS(result) != 0) {
		return SYNC_ERROR_NETWORK;
	}
	
	/* Update sync time in metadata */
	cache_metadata_update_sync(repo->cache_path);
	
	return SYNC_SUCCESS;
}

/**
 * @brief Synchronize repository with all configured mirrors
 */
int sync_with_mirrors(struct repo_info *repo, const struct sync_config *config,
	                 struct sync_result *result)
{
	(void)config; /* Unused for now */
	
	if (!repo || !repo->cache_path || !result) {
		return SYNC_ERROR_INVALID;
	}
	
	/* Initialize result structure */
	memset(result, 0, sizeof(*result));
	result->start_time = time(NULL);
	
	/* Get list of configured remotes */
	char list_remotes_cmd[4096];
	snprintf(list_remotes_cmd, sizeof(list_remotes_cmd),
	         "cd \"%s\" && git remote", repo->cache_path);
	
	FILE *pipe = popen(list_remotes_cmd, "r");
	if (!pipe) {
		return SYNC_ERROR_NETWORK;
	}
	
	char remote_name[256];
	while (fgets(remote_name, sizeof(remote_name), pipe)) {
		/* Remove newline */
		size_t len = strlen(remote_name);
		if (len > 0 && remote_name[len-1] == '\n') {
			remote_name[len-1] = '\0';
		}
		
		/* Skip origin remote for sync operations */
		if (strcmp(remote_name, "origin") == 0) {
			continue;
		}
		
		/* Sync with this mirror */
		int sync_result = sync_with_mirror(repo, remote_name, 0);
		if (sync_result == SYNC_SUCCESS) {
			result->success_count++;
		} else {
			result->error_count++;
		}
	}
	
	pclose(pipe);
	result->end_time = time(NULL);
	
	return result->error_count > 0 ? SYNC_ERROR_NETWORK : SYNC_SUCCESS;
}

/**
 * @brief Push repository changes to all mirrors
 */
int push_to_mirrors(struct repo_info *repo, const char *branch, int force)
{
	if (!repo || !repo->cache_path) {
		return SYNC_ERROR_INVALID;
	}
	
	/* Get list of configured remotes */
	char list_remotes_cmd[4096];
	snprintf(list_remotes_cmd, sizeof(list_remotes_cmd),
	         "cd \"%s\" && git remote", repo->cache_path);
	
	FILE *pipe = popen(list_remotes_cmd, "r");
	if (!pipe) {
		return SYNC_ERROR_NETWORK;
	}
	
	int success_count = 0;
	int error_count = 0;
	char remote_name[256];
	
	while (fgets(remote_name, sizeof(remote_name), pipe)) {
		/* Remove newline */
		size_t len = strlen(remote_name);
		if (len > 0 && remote_name[len-1] == '\n') {
			remote_name[len-1] = '\0';
		}
		
		/* Push to this mirror */
		char push_cmd[8192];
		snprintf(push_cmd, sizeof(push_cmd),
		         "cd \"%s\" && git push %s \"%s\" %s 2>/dev/null",
		         repo->cache_path, force ? "--force" : "", remote_name,
		         branch ? branch : "--all");
		
		int result = system(push_cmd);
		if (WEXITSTATUS(result) == 0) {
			success_count++;
		} else {
			error_count++;
		}
	}
	
	pclose(pipe);
	
	return error_count > 0 ? SYNC_ERROR_NETWORK : SYNC_SUCCESS;
}

/**
 * @brief Clone from best available mirror with fallback
 */
int clone_from_best_mirror(const char *url, const char *target_path,
	                      enum clone_strategy strategy, const char **fallback_mirrors,
	                      int fallback_count)
{
	if (!url || !target_path) {
		return SYNC_ERROR_INVALID;
	}
	
	/* Build strategy arguments */
	char strategy_args[256] = "";
	switch (strategy) {
		case CLONE_STRATEGY_SHALLOW:
			strcpy(strategy_args, "--depth=1");
			break;
		case CLONE_STRATEGY_TREELESS:
			strcpy(strategy_args, "--filter=tree:0");
			break;
		case CLONE_STRATEGY_BLOBLESS:
			strcpy(strategy_args, "--filter=blob:none");
			break;
		case CLONE_STRATEGY_FULL:
		case CLONE_STRATEGY_AUTO:
		default:
			/* No additional args */
			break;
	}
	
	/* Try primary URL first */
	char clone_cmd[8192];
	snprintf(clone_cmd, sizeof(clone_cmd),
	         "git clone %s \"%s\" \"%s\" 2>/dev/null",
	         strategy_args, url, target_path);
	
	int result = system(clone_cmd);
	if (WEXITSTATUS(result) == 0) {
		return SYNC_SUCCESS;
	}
	
	/* Try fallback mirrors */
	for (int i = 0; i < fallback_count; i++) {
		if (!fallback_mirrors[i]) {
			continue;
		}
		
		/* Remove failed attempt */
		char rm_cmd[4096];
		snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", target_path);
		system(rm_cmd);
		
		/* Try fallback mirror */
		snprintf(clone_cmd, sizeof(clone_cmd),
		         "git clone %s \"%s\" \"%s\" 2>/dev/null",
		         strategy_args, fallback_mirrors[i], target_path);
		
		result = system(clone_cmd);
		if (WEXITSTATUS(result) == 0) {
			return SYNC_SUCCESS;
		}
	}
	
	return SYNC_ERROR_NETWORK;
}

/**
 * @brief Test connectivity to mirror
 */
__attribute__((unused))
static int test_mirror_connection(const char *mirror_url)
{
	if (!mirror_url) {
		return 0;
	}
	
	/* Use git ls-remote to test connectivity */
	char test_cmd[4096];
	snprintf(test_cmd, sizeof(test_cmd),
	         "git ls-remote \"%s\" HEAD >/dev/null 2>&1", mirror_url);
	
	int result = system(test_cmd);
	return WEXITSTATUS(result) == 0 ? 1 : 0;
}

/**
 * @brief Get optimal mirror based on availability
 */
const char* get_optimal_mirror(const struct repo_info *repo, const char *operation_type)
{
	(void)operation_type; /* Unused for now */
	
	if (!repo || !repo->cache_path) {
		return NULL;
	}
	
	/* For simplicity, return the first available remote that's not origin */
	char list_remotes_cmd[4096];
	snprintf(list_remotes_cmd, sizeof(list_remotes_cmd),
	         "cd \"%s\" && git remote", repo->cache_path);
	
	FILE *pipe = popen(list_remotes_cmd, "r");
	if (!pipe) {
		return NULL;
	}
	
	static char best_remote[256];
	char remote_name[256];
	
	while (fgets(remote_name, sizeof(remote_name), pipe)) {
		/* Remove newline */
		size_t len = strlen(remote_name);
		if (len > 0 && remote_name[len-1] == '\n') {
			remote_name[len-1] = '\0';
		}
		
		/* Skip origin for mirror operations */
		if (strcmp(remote_name, "origin") == 0) {
			continue;
		}
		
		/* Use first available mirror */
		strcpy(best_remote, remote_name);
		pclose(pipe);
		return best_remote;
	}
	
	pclose(pipe);
	return NULL;
}

/**
 * @brief Check if repository needs synchronization
 */
int needs_synchronization(const struct repo_info *repo, const char *mirror_name)
{
	(void)mirror_name; /* Unused for now */
	
	if (!repo || !repo->cache_path) {
		return SYNC_ERROR_INVALID;
	}
	
	/* Load metadata to check last sync time */
	struct cache_metadata metadata;
	if (cache_metadata_load(repo->cache_path, &metadata) != METADATA_SUCCESS) {
		return 1; /* Assume sync needed if no metadata */
	}
	
	time_t now = time(NULL);
	time_t last_sync = metadata.last_sync_time;
	
	/* Free loaded metadata */
	free(metadata.original_url);
	free(metadata.fork_url);
	free(metadata.owner);
	free(metadata.name);
	free(metadata.fork_organization);
	free(metadata.default_branch);
	
	/* Check if sync is needed (more than 24 hours since last sync) */
	return (now - last_sync) > (24 * 60 * 60) ? 1 : 0;
}

/**
 * @brief Clean up remote mirror list
 */
void cleanup_remote_mirrors(struct remote_mirror *mirrors)
{
	while (mirrors) {
		struct remote_mirror *next = mirrors->next;
		free(mirrors->name);
		free(mirrors->url);
		free(mirrors->type);
		free(mirrors->sync_error);
		free(mirrors);
		mirrors = next;
	}
}

/**
 * @brief Clean up sync result structure
 */
void cleanup_sync_result(struct sync_result *result)
{
	if (!result) {
		return;
	}
	
	free(result->error_summary);
	memset(result, 0, sizeof(*result));
}

/**
 * @brief Clean up sync configuration
 */
void cleanup_sync_config(struct sync_config *config)
{
	if (!config) {
		return;
	}
	
	free(config->preferred_mirror);
	memset(config, 0, sizeof(*config));
}

/**
 * @brief Get human-readable error message for sync error code
 */
const char* sync_get_error_string(int error_code)
{
	switch (error_code) {
		case SYNC_SUCCESS:
			return "Synchronization successful";
		case SYNC_ERROR_NETWORK:
			return "Network error during synchronization";
		case SYNC_ERROR_AUTH:
			return "Authentication error";
		case SYNC_ERROR_NOT_FOUND:
			return "Mirror not found";
		case SYNC_ERROR_CONFLICT:
			return "Synchronization conflict";
		case SYNC_ERROR_TIMEOUT:
			return "Synchronization timeout";
		case SYNC_ERROR_INVALID:
			return "Invalid parameters";
		case SYNC_ERROR_MEMORY:
			return "Memory allocation error";
		default:
			return "Unknown synchronization error";
	}
}