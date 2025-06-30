/**
 * @file cache_recovery.c
 * @brief Cache corruption detection and recovery for git-cache
 * 
 * This module provides functionality for detecting and recovering from
 * corrupted cache repositories and checkout directories.
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
#include "cache_recovery.h"

/**
 * @brief Check if a Git repository is valid and not corrupted
 */
int verify_git_repository(const char *repo_path)
{
	if (!repo_path || access(repo_path, F_OK) != 0) {
		return CACHE_RECOVERY_NOT_EXISTS;
	}
	
	/* Check if it's a Git repository */
	char git_dir[4096];
	snprintf(git_dir, sizeof(git_dir), "%s/.git", repo_path);
	if (access(git_dir, F_OK) != 0) {
		/* Try bare repository */
		snprintf(git_dir, sizeof(git_dir), "%s/refs", repo_path);
		if (access(git_dir, F_OK) != 0) {
			return CACHE_RECOVERY_NOT_GIT_REPO;
		}
	}
	
	/* Run git fsck to verify repository integrity */
	char fsck_cmd[4096];
	snprintf(fsck_cmd, sizeof(fsck_cmd), "cd %s && git fsck --quiet 2>/dev/null", repo_path);
	
	int status = system(fsck_cmd);
	if (status != 0) {
		return CACHE_RECOVERY_CORRUPTED;
	}
	
	return CACHE_RECOVERY_OK;
}

/**
 * @brief Verify cache repository integrity
 */
int verify_cache_repository(const char *cache_path)
{
	if (!cache_path) {
		return CACHE_RECOVERY_INVALID_PATH;
	}
	
	/* Check basic Git repository validity */
	int basic_check = verify_git_repository(cache_path);
	if (basic_check != CACHE_RECOVERY_OK) {
		return basic_check;
	}
	
	/* Additional checks specific to bare repositories */
	char refs_dir[4096];
	snprintf(refs_dir, sizeof(refs_dir), "%s/refs/heads", cache_path);
	if (access(refs_dir, F_OK) != 0) {
		return CACHE_RECOVERY_MISSING_REFS;
	}
	
	/* Check if we have at least one branch */
	DIR *refs = opendir(refs_dir);
	if (!refs) {
		return CACHE_RECOVERY_MISSING_REFS;
	}
	
	const struct dirent *entry;
	int has_branches = 0;
	while ((entry = readdir(refs)) != NULL) {
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
			has_branches = 1;
			break;
		}
	}
	closedir(refs);
	
	if (!has_branches) {
		return CACHE_RECOVERY_EMPTY_REPO;
	}
	
	return CACHE_RECOVERY_OK;
}

/**
 * @brief Verify checkout repository integrity
 */
int verify_checkout_repository(const char *checkout_path, const char *cache_path)
{
	if (!checkout_path || !cache_path) {
		return CACHE_RECOVERY_INVALID_PATH;
	}
	
	/* Check basic Git repository validity */
	int basic_check = verify_git_repository(checkout_path);
	if (basic_check != CACHE_RECOVERY_OK) {
		return basic_check;
	}
	
	/* Check if alternates file exists and points to correct cache */
	char alternates_path[4096];
	snprintf(alternates_path, sizeof(alternates_path), "%s/.git/objects/info/alternates", checkout_path);
	
	FILE *alternates_file = fopen(alternates_path, "r");
	if (!alternates_file) {
		return CACHE_RECOVERY_NO_ALTERNATES;
	}
	
	char line[4096];
	int found_cache = 0;
	while (fgets(line, sizeof(line), alternates_file)) {
		/* Remove trailing newline */
		size_t len = strlen(line);
		if (len > 0 && line[len-1] == '\n') {
			line[len-1] = '\0';
		}
		
		/* Check if this line points to our cache */
		char expected_objects[4096];
		snprintf(expected_objects, sizeof(expected_objects), "%s/objects", cache_path);
		if (strcmp(line, expected_objects) == 0) {
			found_cache = 1;
			break;
		}
	}
	fclose(alternates_file);
	
	if (!found_cache) {
		return CACHE_RECOVERY_WRONG_ALTERNATES;
	}
	
	return CACHE_RECOVERY_OK;
}

/**
 * @brief Attempt to repair a corrupted cache repository
 */
int repair_cache_repository(const char *cache_path, const char *original_url, int verbose)
{
	if (!cache_path || !original_url) {
		return CACHE_RECOVERY_INVALID_PATH;
	}
	
	if (verbose) {
		printf("Attempting to repair cache repository: %s\n", cache_path);
	}
	
	/* Create backup of corrupted cache */
	char backup_path[4096];
	snprintf(backup_path, sizeof(backup_path), "%s.corrupted.%ld", cache_path, (long)time(NULL));
	
	char backup_cmd[8192];
	snprintf(backup_cmd, sizeof(backup_cmd), "mv %s %s", cache_path, backup_path);
	if (system(backup_cmd) != 0) {
		if (verbose) {
			printf("Warning: Could not backup corrupted cache\n");
		}
	}
	
	/* Re-clone the repository */
	char clone_cmd[8192];
	snprintf(clone_cmd, sizeof(clone_cmd), "git clone --bare %s %s %s",
		verbose ? "" : "-q", original_url, cache_path);
	
	if (system(clone_cmd) != 0) {
		if (verbose) {
			printf("Failed to re-clone repository\n");
		}
		/* Restore backup if re-clone failed */
		snprintf(backup_cmd, sizeof(backup_cmd), "mv %s %s", backup_path, cache_path);
		if (system(backup_cmd) != 0) {
			if (verbose) {
				printf("Critical: Could not restore backup!\n");
			}
		}
		return CACHE_RECOVERY_REPAIR_FAILED;
	}
	
	/* Verify the repaired repository */
	if (verify_cache_repository(cache_path) != CACHE_RECOVERY_OK) {
		if (verbose) {
			printf("Repaired repository still appears corrupted\n");
		}
		return CACHE_RECOVERY_REPAIR_FAILED;
	}
	
	if (verbose) {
		printf("Cache repository repaired successfully\n");
		printf("Corrupted cache backed up to: %s\n", backup_path);
	}
	
	return CACHE_RECOVERY_OK;
}

/**
 * @brief Repair a checkout repository by recreating it from cache
 */
int repair_checkout_repository(const char *checkout_path, const char *cache_path, 
	                          enum clone_strategy strategy, int verbose)
{
	if (!checkout_path || !cache_path) {
		return CACHE_RECOVERY_INVALID_PATH;
	}
	
	if (verbose) {
		printf("Repairing checkout repository: %s\n", checkout_path);
	}
	
	/* Remove corrupted checkout */
	char remove_cmd[4096];
	snprintf(remove_cmd, sizeof(remove_cmd), "rm -rf %s", checkout_path);
	if (system(remove_cmd) != 0) {
		if (verbose) {
			printf("Warning: Could not remove corrupted checkout\n");
		}
	}
	
	/* Recreate checkout from cache */
	char clone_cmd[8192];
	const char *strategy_flag = "";
	
	switch (strategy) {
		case CLONE_STRATEGY_SHALLOW:
			strategy_flag = "--depth=1";
			break;
		case CLONE_STRATEGY_TREELESS:
			strategy_flag = "--filter=tree:0";
			break;
		case CLONE_STRATEGY_BLOBLESS:
			strategy_flag = "--filter=blob:none";
			break;
		case CLONE_STRATEGY_FULL:
		default:
			strategy_flag = "";
			break;
	}
	
	snprintf(clone_cmd, sizeof(clone_cmd), 
		"git clone %s --reference=%s %s %s %s",
		verbose ? "" : "-q",
		cache_path,
		strategy_flag,
		cache_path,
		checkout_path);
	
	if (system(clone_cmd) != 0) {
		if (verbose) {
			printf("Failed to recreate checkout from cache\n");
		}
		return CACHE_RECOVERY_REPAIR_FAILED;
	}
	
	/* Verify the repaired checkout */
	if (verify_checkout_repository(checkout_path, cache_path) != CACHE_RECOVERY_OK) {
		if (verbose) {
			printf("Repaired checkout still appears corrupted\n");
		}
		return CACHE_RECOVERY_REPAIR_FAILED;
	}
	
	if (verbose) {
		printf("Checkout repository repaired successfully\n");
	}
	
	return CACHE_RECOVERY_OK;
}

/**
 * @brief Comprehensive verification and repair of repository and checkouts
 */
int verify_and_repair_repository(struct repo_info *repo, const struct cache_config *config)
{
	if (!repo || !config) {
		return CACHE_RECOVERY_INVALID_PATH;
	}
	
	int cache_status = verify_cache_repository(repo->cache_path);
	int checkout_status = CACHE_RECOVERY_OK;
	int modifiable_status = CACHE_RECOVERY_OK;
	
	/* Verify cache repository */
	if (cache_status != CACHE_RECOVERY_OK) {
		if (config->verbose) {
			printf("Cache repository corrupted (%s): %s\n", 
				cache_recovery_error_string(cache_status), repo->cache_path);
		}
		
		/* Attempt repair */
		int repair_result = repair_cache_repository(repo->cache_path, repo->original_url, config->verbose);
		if (repair_result != CACHE_RECOVERY_OK) {
			return repair_result;
		}
		
		/* Cache was repaired, need to repair checkouts too */
		checkout_status = CACHE_RECOVERY_CORRUPTED;
		modifiable_status = CACHE_RECOVERY_CORRUPTED;
	}
	
	/* Verify read-only checkout */
	if (repo->checkout_path && access(repo->checkout_path, F_OK) == 0) {
		if (checkout_status == CACHE_RECOVERY_OK) {
			checkout_status = verify_checkout_repository(repo->checkout_path, repo->cache_path);
		}
		
		if (checkout_status != CACHE_RECOVERY_OK) {
			if (config->verbose) {
				printf("Read-only checkout corrupted (%s): %s\n",
					cache_recovery_error_string(checkout_status), repo->checkout_path);
			}
			
			int repair_result = repair_checkout_repository(repo->checkout_path, repo->cache_path, 
				repo->strategy, config->verbose);
			if (repair_result != CACHE_RECOVERY_OK) {
				printf("Warning: Could not repair read-only checkout\n");
			}
		}
	}
	
	/* Verify modifiable checkout */
	if (repo->modifiable_path && access(repo->modifiable_path, F_OK) == 0) {
		if (modifiable_status == CACHE_RECOVERY_OK) {
			modifiable_status = verify_checkout_repository(repo->modifiable_path, repo->cache_path);
		}
		
		if (modifiable_status != CACHE_RECOVERY_OK) {
			if (config->verbose) {
				printf("Modifiable checkout corrupted (%s): %s\n",
					cache_recovery_error_string(modifiable_status), repo->modifiable_path);
			}
			
			int repair_result = repair_checkout_repository(repo->modifiable_path, repo->cache_path, 
				repo->strategy, config->verbose);
			if (repair_result != CACHE_RECOVERY_OK) {
				printf("Warning: Could not repair modifiable checkout\n");
			}
		}
	}
	
	return CACHE_RECOVERY_OK;
}

/**
 * @brief Get human-readable error message for recovery status
 */
const char* cache_recovery_error_string(int error_code)
{
	switch (error_code) {
		case CACHE_RECOVERY_OK:
			return "Repository is valid";
		case CACHE_RECOVERY_NOT_EXISTS:
			return "Repository does not exist";
		case CACHE_RECOVERY_NOT_GIT_REPO:
			return "Not a Git repository";
		case CACHE_RECOVERY_CORRUPTED:
			return "Repository is corrupted";
		case CACHE_RECOVERY_MISSING_REFS:
			return "Missing Git references";
		case CACHE_RECOVERY_EMPTY_REPO:
			return "Repository has no branches";
		case CACHE_RECOVERY_NO_ALTERNATES:
			return "Missing alternates file";
		case CACHE_RECOVERY_WRONG_ALTERNATES:
			return "Incorrect alternates configuration";
		case CACHE_RECOVERY_INVALID_PATH:
			return "Invalid repository path";
		case CACHE_RECOVERY_REPAIR_FAILED:
			return "Repository repair failed";
		default:
			return "Unknown error";
	}
}