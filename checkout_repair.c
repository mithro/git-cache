/**
 * @file checkout_repair.c
 * @brief Implementation of checkout repair mechanisms
 * 
 * Provides functionality for detecting and repairing checkouts that have
 * become outdated or corrupted when the cache is updated.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#include "git-cache.h"
#include "checkout_repair.h"
#include "cache_recovery.h"
#include "cache_metadata.h"

/**
 * @brief Get modification time of a directory
 */
static int get_directory_mtime(const char *path, time_t *mtime)
{
	if (!path || !mtime) {
		return -1;
	}
	
	struct stat st;
	if (stat(path, &st) != 0) {
		return -1;
	}
	
	*mtime = st.st_mtime;
	return 0;
}

/**
 * @brief Check if cache has been updated since checkout was created
 */
int cache_newer_than_checkout(const char *checkout_path, const char *cache_path,
	                         time_t *checkout_mtime, time_t *cache_mtime)
{
	if (!checkout_path || !cache_path) {
		return CHECKOUT_REPAIR_INVALID_ARGS;
	}
	
	time_t checkout_time, cache_time;
	
	/* Get checkout modification time */
	char checkout_git[4096];
	snprintf(checkout_git, sizeof(checkout_git), "%s/.git", checkout_path);
	if (get_directory_mtime(checkout_git, &checkout_time) != 0) {
		return CHECKOUT_REPAIR_IO_ERROR;
	}
	
	/* Get cache modification time */
	char cache_refs[4096];
	snprintf(cache_refs, sizeof(cache_refs), "%s/refs", cache_path);
	if (get_directory_mtime(cache_refs, &cache_time) != 0) {
		return CHECKOUT_REPAIR_NO_CACHE;
	}
	
	if (checkout_mtime) *checkout_mtime = checkout_time;
	if (cache_mtime) *cache_mtime = cache_time;
	
	return cache_time > checkout_time ? 1 : 0;
}

/**
 * @brief Check if checkout needs repair due to cache updates
 */
int checkout_needs_repair(const char *checkout_path, const char *cache_path, time_t last_cache_sync)
{
	if (!checkout_path || !cache_path) {
		return CHECKOUT_REPAIR_INVALID_ARGS;
	}
	
	/* First check if checkout exists and is valid */
	int checkout_status = verify_checkout_repository(checkout_path, cache_path);
	if (checkout_status != CACHE_RECOVERY_OK) {
		return 1; /* Needs repair */
	}
	
	/* Check if cache is newer than checkout */
	time_t checkout_time, cache_time;
	int newer = cache_newer_than_checkout(checkout_path, cache_path, &checkout_time, &cache_time);
	if (newer < 0) {
		return newer; /* Error */
	}
	
	if (newer > 0) {
		/* Cache has been updated after checkout was created */
		return 1;
	}
	
	/* If we have last sync time, check if checkout is older */
	if (last_cache_sync > 0 && checkout_time < last_cache_sync) {
		return 1;
	}
	
	/* Check if checkout has uncommitted changes */
	char status_cmd[8192];
	snprintf(status_cmd, sizeof(status_cmd), 
	         "cd \"%s\" && git status --porcelain 2>/dev/null | grep -q '^'", 
	         checkout_path);
	
	int has_changes = system(status_cmd) == 0;
	if (has_changes) {
		/* Has uncommitted changes, don't repair automatically */
		return 0;
	}
	
	/* Check if checkout is behind cache */
	char behind_cmd[8192];
	snprintf(behind_cmd, sizeof(behind_cmd),
	         "cd \"%s\" && git rev-list HEAD..origin/HEAD --count 2>/dev/null",
	         checkout_path);
	
	FILE *pipe = popen(behind_cmd, "r");
	if (pipe) {
		int behind_count = 0;
		if (fscanf(pipe, "%d", &behind_count) == 1 && behind_count > 0) {
			pclose(pipe);
			return 1; /* Behind origin, needs update */
		}
		pclose(pipe);
	}
	
	return 0; /* No repair needed */
}

/**
 * @brief Update checkout to latest state from cache
 */
int update_checkout_from_cache(const char *checkout_path, const char *cache_path, int verbose)
{
	if (!checkout_path || !cache_path) {
		return CHECKOUT_REPAIR_INVALID_ARGS;
	}
	
	if (verbose) {
		printf("Updating checkout from cache: %s\n", checkout_path);
	}
	
	/* Fetch latest changes from cache */
	char fetch_cmd[8192];
	snprintf(fetch_cmd, sizeof(fetch_cmd),
	         "cd \"%s\" && git fetch origin 2>&1",
	         checkout_path);
	
	int fetch_result = system(fetch_cmd);
	if (WEXITSTATUS(fetch_result) != 0) {
		if (verbose) {
			printf("Failed to fetch from cache\n");
		}
		return CHECKOUT_REPAIR_FAILED;
	}
	
	/* Reset to origin/HEAD */
	char reset_cmd[8192];
	snprintf(reset_cmd, sizeof(reset_cmd),
	         "cd \"%s\" && git reset --hard origin/HEAD 2>&1",
	         checkout_path);
	
	int reset_result = system(reset_cmd);
	if (WEXITSTATUS(reset_result) != 0) {
		if (verbose) {
			printf("Failed to reset to origin/HEAD\n");
		}
		return CHECKOUT_REPAIR_FAILED;
	}
	
	/* Clean untracked files */
	char clean_cmd[8192];
	snprintf(clean_cmd, sizeof(clean_cmd),
	         "cd \"%s\" && git clean -fd 2>&1",
	         checkout_path);
	
	if (system(clean_cmd) != 0) {
		if (verbose) {
			printf("Warning: Failed to clean untracked files\n");
		}
	}
	
	if (verbose) {
		printf("Checkout updated successfully\n");
	}
	
	return CHECKOUT_REPAIR_SUCCESS;
}

/**
 * @brief Repair a checkout that has become outdated
 */
int repair_outdated_checkout(const char *checkout_path, const char *cache_path, 
	                        enum clone_strategy strategy, int verbose)
{
	if (!checkout_path || !cache_path) {
		return CHECKOUT_REPAIR_INVALID_ARGS;
	}
	
	if (verbose) {
		printf("Repairing outdated checkout: %s\n", checkout_path);
	}
	
	/* First try to update the checkout */
	int update_result = update_checkout_from_cache(checkout_path, cache_path, verbose);
	if (update_result == CHECKOUT_REPAIR_SUCCESS) {
		return CHECKOUT_REPAIR_SUCCESS;
	}
	
	/* If update failed, try full repair */
	if (verbose) {
		printf("Update failed, attempting full repair\n");
	}
	
	/* Use the cache recovery system to repair */
	int repair_result = repair_checkout_repository(checkout_path, cache_path, strategy, verbose);
	if (repair_result != CACHE_RECOVERY_OK) {
		if (verbose) {
			printf("Checkout repair failed: %s\n", cache_recovery_error_string(repair_result));
		}
		return CHECKOUT_REPAIR_FAILED;
	}
	
	return CHECKOUT_REPAIR_SUCCESS;
}

/**
 * @brief Check if checkout references are valid and point to correct cache
 */
int validate_checkout_references(const char *checkout_path, const char *expected_cache_path)
{
	if (!checkout_path || !expected_cache_path) {
		return -1;
	}
	
	/* Check alternates file */
	char alternates_path[4096];
	snprintf(alternates_path, sizeof(alternates_path), 
	         "%s/.git/objects/info/alternates", checkout_path);
	
	FILE *alternates = fopen(alternates_path, "r");
	if (!alternates) {
		return 0; /* No alternates file */
	}
	
	char line[4096];
	int found_correct = 0;
	while (fgets(line, sizeof(line), alternates)) {
		/* Remove newline */
		size_t len = strlen(line);
		if (len > 0 && line[len-1] == '\n') {
			line[len-1] = '\0';
		}
		
		/* Check if this points to our expected cache */
		char expected_objects[4096];
		snprintf(expected_objects, sizeof(expected_objects), "%s/objects", expected_cache_path);
		if (strcmp(line, expected_objects) == 0) {
			found_correct = 1;
			break;
		}
	}
	
	fclose(alternates);
	return found_correct;
}

/**
 * @brief Check and repair all checkouts for a repository
 */
int repair_all_checkouts_for_repo(struct repo_info *repo, struct cache_config *config)
{
	if (!repo || !config) {
		return CHECKOUT_REPAIR_INVALID_ARGS;
	}
	
	int repaired_count = 0;
	
	/* Get last sync time from metadata */
	struct cache_metadata metadata;
	time_t last_sync = 0;
	if (cache_metadata_load(repo->cache_path, &metadata) == METADATA_SUCCESS) {
		last_sync = metadata.last_sync_time;
		
		/* Clean up loaded metadata */
		free(metadata.original_url);
		free(metadata.fork_url);
		free(metadata.owner);
		free(metadata.name);
		free(metadata.fork_organization);
		free(metadata.default_branch);
	}
	
	/* Check read-only checkout */
	if (repo->checkout_path && access(repo->checkout_path, F_OK) == 0) {
		int needs_repair = checkout_needs_repair(repo->checkout_path, repo->cache_path, last_sync);
		if (needs_repair > 0) {
			if (config->verbose) {
				printf("Repairing read-only checkout: %s\n", repo->checkout_path);
			}
			
			int result = repair_outdated_checkout(repo->checkout_path, repo->cache_path, 
			                                     repo->strategy, config->verbose);
			if (result == CHECKOUT_REPAIR_SUCCESS) {
				repaired_count++;
			}
		}
	}
	
	/* Check modifiable checkout */
	if (repo->modifiable_path && access(repo->modifiable_path, F_OK) == 0) {
		int needs_repair = checkout_needs_repair(repo->modifiable_path, repo->cache_path, last_sync);
		if (needs_repair > 0) {
			if (config->verbose) {
				printf("Repairing modifiable checkout: %s\n", repo->modifiable_path);
			}
			
			int result = repair_outdated_checkout(repo->modifiable_path, repo->cache_path, 
			                                     repo->strategy, config->verbose);
			if (result == CHECKOUT_REPAIR_SUCCESS) {
				repaired_count++;
			}
		}
	}
	
	return repaired_count;
}

/**
 * @brief Callback structure for listing repositories
 */
struct repair_callback_data {
	struct cache_config *config;
	int force_repair;
	int repaired_count;
	int error_count;
};

/**
 * @brief Callback for repairing checkouts of a repository
 */
static int repair_repo_callback(const struct cache_metadata *metadata, void *user_data)
{
	struct repair_callback_data *data = (struct repair_callback_data *)user_data;
	
	/* Build repo info from metadata */
	struct repo_info repo;
	memset(&repo, 0, sizeof(repo));
	
	repo.original_url = metadata->original_url;
	repo.owner = metadata->owner;
	repo.name = metadata->name;
	repo.type = metadata->type;
	repo.strategy = metadata->strategy;
	
	/* Build paths */
	char cache_path[4096];
	snprintf(cache_path, sizeof(cache_path), "%s/github.com/%s/%s", 
	         data->config->cache_root, metadata->owner, metadata->name);
	repo.cache_path = cache_path;
	
	char checkout_path[4096];
	snprintf(checkout_path, sizeof(checkout_path), "%s/%s/%s", 
	         data->config->checkout_root, metadata->owner, metadata->name);
	repo.checkout_path = checkout_path;
	
	/* Check if modifiable checkout exists */
	char modifiable_path[4096];
	snprintf(modifiable_path, sizeof(modifiable_path), "%s/mithro/%s", 
	         data->config->checkout_root, metadata->name);
	if (access(modifiable_path, F_OK) == 0) {
		repo.modifiable_path = modifiable_path;
	}
	
	/* Repair checkouts for this repository */
	int result = repair_all_checkouts_for_repo(&repo, data->config);
	if (result > 0) {
		data->repaired_count += result;
	} else if (result < 0) {
		data->error_count++;
	}
	
	return 0; /* Continue iteration */
}

/**
 * @brief Find and repair all outdated checkouts in the system
 */
int repair_all_outdated_checkouts(struct cache_config *config, int force_repair)
{
	if (!config) {
		return CHECKOUT_REPAIR_INVALID_ARGS;
	}
	
	struct repair_callback_data data = {
		.config = config,
		.force_repair = force_repair,
		.repaired_count = 0,
		.error_count = 0
	};
	
	/* Iterate through all cached repositories */
	int count = cache_metadata_list_all(config, repair_repo_callback, &data);
	if (count < 0) {
		return count;
	}
	
	if (config->verbose) {
		printf("Checked %d repositories, repaired %d checkouts, %d errors\n", 
		       count, data.repaired_count, data.error_count);
	}
	
	return data.repaired_count;
}

/**
 * @brief Detect orphaned checkouts (cache no longer exists)
 */
int detect_orphaned_checkouts(struct cache_config *config,
	                         int (*callback)(const char *checkout_path, void *user_data),
	                         void *user_data)
{
	if (!config || !callback) {
		return CHECKOUT_REPAIR_INVALID_ARGS;
	}
	
	int orphaned_count = 0;
	
	/* Walk through checkout directory */
	DIR *checkout_root = opendir(config->checkout_root);
	if (!checkout_root) {
		return 0;
	}
	
	const struct dirent *owner_entry;
	while ((owner_entry = readdir(checkout_root)) != NULL) {
		if (strcmp(owner_entry->d_name, ".") == 0 || 
		    strcmp(owner_entry->d_name, "..") == 0) {
			continue;
		}
		
		char owner_path[4096];
		snprintf(owner_path, sizeof(owner_path), "%s/%s", 
		         config->checkout_root, owner_entry->d_name);
		
		DIR *owner_dir = opendir(owner_path);
		if (!owner_dir) {
			continue;
		}
		
		const struct dirent *repo_entry;
		while ((repo_entry = readdir(owner_dir)) != NULL) {
			if (strcmp(repo_entry->d_name, ".") == 0 || 
			    strcmp(repo_entry->d_name, "..") == 0) {
				continue;
			}
			
			char checkout_path[4096];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
			snprintf(checkout_path, sizeof(checkout_path), "%s/%s", 
			         owner_path, repo_entry->d_name);
#pragma GCC diagnostic pop
			
			/* Check if this is a git repository */
			char git_dir[4096];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
			snprintf(git_dir, sizeof(git_dir), "%s/.git", checkout_path);
#pragma GCC diagnostic pop
			if (access(git_dir, F_OK) != 0) {
				continue;
			}
			
			/* Build expected cache path */
			char expected_cache[4096];
			snprintf(expected_cache, sizeof(expected_cache), "%s/github.com/%s/%s",
			         config->cache_root, owner_entry->d_name, repo_entry->d_name);
			
			/* Check if cache exists */
			if (access(expected_cache, F_OK) != 0) {
				/* Cache doesn't exist, this is an orphaned checkout */
				orphaned_count++;
				if (callback(checkout_path, user_data) != 0) {
					closedir(owner_dir);
					closedir(checkout_root);
					return orphaned_count;
				}
			}
		}
		closedir(owner_dir);
	}
	closedir(checkout_root);
	
	return orphaned_count;
}

/**
 * @brief Get checkout repair status message
 */
const char* checkout_repair_status_string(int status_code)
{
	switch (status_code) {
		case CHECKOUT_REPAIR_SUCCESS:
			return "Checkout repaired successfully";
		case CHECKOUT_REPAIR_NOT_NEEDED:
			return "Checkout repair not needed";
		case CHECKOUT_REPAIR_FAILED:
			return "Checkout repair failed";
		case CHECKOUT_REPAIR_INVALID_ARGS:
			return "Invalid arguments";
		case CHECKOUT_REPAIR_NO_CACHE:
			return "Cache repository not found";
		case CHECKOUT_REPAIR_IO_ERROR:
			return "I/O error";
		default:
			return "Unknown error";
	}
}