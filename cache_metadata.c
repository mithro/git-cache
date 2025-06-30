/**
 * @file cache_metadata.c
 * @brief Cache metadata storage and retrieval implementation
 * 
 * This module implements storage and retrieval of metadata for cached
 * repositories using JSON format for persistence.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <json-c/json.h>

#include "git-cache.h"
#include "cache_metadata.h"

/* Metadata file name */
#define METADATA_FILE "cache_metadata.json"

/**
 * @brief Create metadata structure from repository info
 */
struct cache_metadata* cache_metadata_create(const struct repo_info *repo)
{
	if (!repo) {
		return NULL;
	}
	
	struct cache_metadata *metadata = calloc(1, sizeof(struct cache_metadata));
	if (!metadata) {
		return NULL;
	}
	
	/* Copy string fields */
	if (repo->original_url) {
		metadata->original_url = strdup(repo->original_url);
	}
	if (repo->fork_url) {
		metadata->fork_url = strdup(repo->fork_url);
	}
	if (repo->owner) {
		metadata->owner = strdup(repo->owner);
	}
	if (repo->name) {
		metadata->name = strdup(repo->name);
	}
	if (repo->fork_organization) {
		metadata->fork_organization = strdup(repo->fork_organization);
	}
	
	/* Copy simple fields */
	metadata->type = repo->type;
	metadata->strategy = repo->strategy;
	metadata->is_fork_needed = repo->is_fork_needed;
	metadata->created_time = time(NULL);
	metadata->last_access_time = metadata->created_time;
	metadata->ref_count = 0;
	
	return metadata;
}

/**
 * @brief Destroy metadata structure
 */
void cache_metadata_destroy(struct cache_metadata *metadata)
{
	if (!metadata) {
		return;
	}
	
	free(metadata->original_url);
	free(metadata->fork_url);
	free(metadata->owner);
	free(metadata->name);
	free(metadata->fork_organization);
	free(metadata->default_branch);
	free(metadata);
}

/**
 * @brief Get metadata file path for cache
 */
int cache_metadata_get_path(const char *cache_path, char *metadata_path, size_t path_size)
{
	if (!cache_path || !metadata_path || path_size == 0) {
		return METADATA_ERROR_INVALID;
	}
	
	int ret = snprintf(metadata_path, path_size, "%s/%s", cache_path, METADATA_FILE);
	if (ret < 0 || (size_t)ret >= path_size) {
		return METADATA_ERROR_INVALID;
	}
	
	return METADATA_SUCCESS;
}

/**
 * @brief Convert clone strategy to string
 */
static const char* strategy_to_string(enum clone_strategy strategy)
{
	switch (strategy) {
		case CLONE_STRATEGY_FULL: return "full";
		case CLONE_STRATEGY_SHALLOW: return "shallow";
		case CLONE_STRATEGY_TREELESS: return "treeless";
		case CLONE_STRATEGY_BLOBLESS: return "blobless";
		default: return "full";
	}
}

/**
 * @brief Convert string to clone strategy
 */
static enum clone_strategy string_to_strategy(const char *str)
{
	if (!str) return CLONE_STRATEGY_FULL;
	
	if (strcmp(str, "shallow") == 0) return CLONE_STRATEGY_SHALLOW;
	if (strcmp(str, "treeless") == 0) return CLONE_STRATEGY_TREELESS;
	if (strcmp(str, "blobless") == 0) return CLONE_STRATEGY_BLOBLESS;
	return CLONE_STRATEGY_FULL;
}

/**
 * @brief Convert repo type to string
 */
static const char* repo_type_to_string(enum repo_type type)
{
	switch (type) {
		case REPO_TYPE_GITHUB: return "github";
		case REPO_TYPE_UNKNOWN: return "unknown";
		default: return "unknown";
	}
}

/**
 * @brief Convert string to repo type
 */
static enum repo_type string_to_repo_type(const char *str)
{
	if (!str) return REPO_TYPE_UNKNOWN;
	
	if (strcmp(str, "github") == 0) return REPO_TYPE_GITHUB;
	return REPO_TYPE_UNKNOWN;
}

/**
 * @brief Save metadata to storage
 */
int cache_metadata_save(const char *cache_path, const struct cache_metadata *metadata)
{
	if (!cache_path || !metadata) {
		return METADATA_ERROR_INVALID;
	}
	
	char metadata_path[4096];
	int ret = cache_metadata_get_path(cache_path, metadata_path, sizeof(metadata_path));
	if (ret != METADATA_SUCCESS) {
		return ret;
	}
	
	/* Create JSON object */
	json_object *root = json_object_new_object();
	if (!root) {
		return METADATA_ERROR_MEMORY;
	}
	
	/* Add string fields */
	if (metadata->original_url) {
		json_object *url_obj = json_object_new_string(metadata->original_url);
		json_object_object_add(root, "original_url", url_obj);
	}
	
	if (metadata->fork_url) {
		json_object *fork_obj = json_object_new_string(metadata->fork_url);
		json_object_object_add(root, "fork_url", fork_obj);
	}
	
	if (metadata->owner) {
		json_object *owner_obj = json_object_new_string(metadata->owner);
		json_object_object_add(root, "owner", owner_obj);
	}
	
	if (metadata->name) {
		json_object *name_obj = json_object_new_string(metadata->name);
		json_object_object_add(root, "name", name_obj);
	}
	
	if (metadata->fork_organization) {
		json_object *org_obj = json_object_new_string(metadata->fork_organization);
		json_object_object_add(root, "fork_organization", org_obj);
	}
	
	if (metadata->default_branch) {
		json_object *branch_obj = json_object_new_string(metadata->default_branch);
		json_object_object_add(root, "default_branch", branch_obj);
	}
	
	/* Add enum fields */
	json_object *type_obj = json_object_new_string(repo_type_to_string(metadata->type));
	json_object_object_add(root, "type", type_obj);
	
	json_object *strategy_obj = json_object_new_string(strategy_to_string(metadata->strategy));
	json_object_object_add(root, "strategy", strategy_obj);
	
	/* Add numeric fields */
	json_object *created_obj = json_object_new_int64(metadata->created_time);
	json_object_object_add(root, "created_time", created_obj);
	
	json_object *sync_obj = json_object_new_int64(metadata->last_sync_time);
	json_object_object_add(root, "last_sync_time", sync_obj);
	
	json_object *access_obj = json_object_new_int64(metadata->last_access_time);
	json_object_object_add(root, "last_access_time", access_obj);
	
	json_object *size_obj = json_object_new_int64(metadata->cache_size);
	json_object_object_add(root, "cache_size", size_obj);
	
	json_object *ref_obj = json_object_new_int(metadata->ref_count);
	json_object_object_add(root, "ref_count", ref_obj);
	
	/* Add boolean fields */
	json_object *fork_needed_obj = json_object_new_boolean(metadata->is_fork_needed);
	json_object_object_add(root, "is_fork_needed", fork_needed_obj);
	
	json_object *private_obj = json_object_new_boolean(metadata->is_private_fork);
	json_object_object_add(root, "is_private_fork", private_obj);
	
	json_object *submodules_obj = json_object_new_boolean(metadata->has_submodules);
	json_object_object_add(root, "has_submodules", submodules_obj);
	
	/* Write to file */
	const char *json_string = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
	if (!json_string) {
		json_object_put(root);
		return METADATA_ERROR_MEMORY;
	}
	
	FILE *file = fopen(metadata_path, "w");
	if (!file) {
		json_object_put(root);
		return METADATA_ERROR_IO;
	}
	
	if (fputs(json_string, file) == EOF) {
		fclose(file);
		json_object_put(root);
		return METADATA_ERROR_IO;
	}
	
	fclose(file);
	json_object_put(root);
	
	return METADATA_SUCCESS;
}

/**
 * @brief Load metadata from storage
 */
int cache_metadata_load(const char *cache_path, struct cache_metadata *metadata)
{
	if (!cache_path || !metadata) {
		return METADATA_ERROR_INVALID;
	}
	
	char metadata_path[4096];
	int ret = cache_metadata_get_path(cache_path, metadata_path, sizeof(metadata_path));
	if (ret != METADATA_SUCCESS) {
		return ret;
	}
	
	/* Check if file exists */
	if (access(metadata_path, F_OK) != 0) {
		return METADATA_ERROR_NOT_FOUND;
	}
	
	/* Read file */
	json_object *root = json_object_from_file(metadata_path);
	if (!root) {
		return METADATA_ERROR_CORRUPT;
	}
	
	/* Initialize metadata structure */
	memset(metadata, 0, sizeof(*metadata));
	
	/* Load string fields */
	json_object *obj;
	if (json_object_object_get_ex(root, "original_url", &obj)) {
		const char *str = json_object_get_string(obj);
		if (str) metadata->original_url = strdup(str);
	}
	
	if (json_object_object_get_ex(root, "fork_url", &obj)) {
		const char *str = json_object_get_string(obj);
		if (str) metadata->fork_url = strdup(str);
	}
	
	if (json_object_object_get_ex(root, "owner", &obj)) {
		const char *str = json_object_get_string(obj);
		if (str) metadata->owner = strdup(str);
	}
	
	if (json_object_object_get_ex(root, "name", &obj)) {
		const char *str = json_object_get_string(obj);
		if (str) metadata->name = strdup(str);
	}
	
	if (json_object_object_get_ex(root, "fork_organization", &obj)) {
		const char *str = json_object_get_string(obj);
		if (str) metadata->fork_organization = strdup(str);
	}
	
	if (json_object_object_get_ex(root, "default_branch", &obj)) {
		const char *str = json_object_get_string(obj);
		if (str) metadata->default_branch = strdup(str);
	}
	
	/* Load enum fields */
	if (json_object_object_get_ex(root, "type", &obj)) {
		const char *str = json_object_get_string(obj);
		metadata->type = string_to_repo_type(str);
	}
	
	if (json_object_object_get_ex(root, "strategy", &obj)) {
		const char *str = json_object_get_string(obj);
		metadata->strategy = string_to_strategy(str);
	}
	
	/* Load numeric fields */
	if (json_object_object_get_ex(root, "created_time", &obj)) {
		metadata->created_time = json_object_get_int64(obj);
	}
	
	if (json_object_object_get_ex(root, "last_sync_time", &obj)) {
		metadata->last_sync_time = json_object_get_int64(obj);
	}
	
	if (json_object_object_get_ex(root, "last_access_time", &obj)) {
		metadata->last_access_time = json_object_get_int64(obj);
	}
	
	if (json_object_object_get_ex(root, "cache_size", &obj)) {
		metadata->cache_size = json_object_get_int64(obj);
	}
	
	if (json_object_object_get_ex(root, "ref_count", &obj)) {
		metadata->ref_count = json_object_get_int(obj);
	}
	
	/* Load boolean fields */
	if (json_object_object_get_ex(root, "is_fork_needed", &obj)) {
		metadata->is_fork_needed = json_object_get_boolean(obj);
	}
	
	if (json_object_object_get_ex(root, "is_private_fork", &obj)) {
		metadata->is_private_fork = json_object_get_boolean(obj);
	}
	
	if (json_object_object_get_ex(root, "has_submodules", &obj)) {
		metadata->has_submodules = json_object_get_boolean(obj);
	}
	
	json_object_put(root);
	return METADATA_SUCCESS;
}

/**
 * @brief Update last access time
 */
int cache_metadata_update_access(const char *cache_path)
{
	if (!cache_path) {
		return METADATA_ERROR_INVALID;
	}
	
	struct cache_metadata metadata;
	int ret = cache_metadata_load(cache_path, &metadata);
	if (ret != METADATA_SUCCESS) {
		return ret;
	}
	
	metadata.last_access_time = time(NULL);
	ret = cache_metadata_save(cache_path, &metadata);
	
	/* Clean up stack-allocated metadata strings */
	free(metadata.original_url);
	free(metadata.fork_url);
	free(metadata.owner);
	free(metadata.name);
	free(metadata.fork_organization);
	free(metadata.default_branch);
	
	return ret;
}

/**
 * @brief Update last sync time
 */
int cache_metadata_update_sync(const char *cache_path)
{
	if (!cache_path) {
		return METADATA_ERROR_INVALID;
	}
	
	struct cache_metadata metadata;
	int ret = cache_metadata_load(cache_path, &metadata);
	if (ret != METADATA_SUCCESS) {
		return ret;
	}
	
	metadata.last_sync_time = time(NULL);
	ret = cache_metadata_save(cache_path, &metadata);
	
	/* Clean up stack-allocated metadata strings */
	free(metadata.original_url);
	free(metadata.fork_url);
	free(metadata.owner);
	free(metadata.name);
	free(metadata.fork_organization);
	free(metadata.default_branch);
	
	return ret;
}

/**
 * @brief Calculate cache directory size
 */
size_t cache_metadata_calculate_size(const char *cache_path)
{
	if (!cache_path) {
		return 0;
	}
	
	/* Use du command to calculate size */
	char cmd[4096];
	snprintf(cmd, sizeof(cmd), "du -sb %s 2>/dev/null | cut -f1", cache_path);
	
	FILE *pipe = popen(cmd, "r");
	if (!pipe) {
		return 0;
	}
	
	char buffer[256];
	size_t size = 0;
	if (fgets(buffer, sizeof(buffer), pipe)) {
		size = strtoull(buffer, NULL, 10);
	}
	
	pclose(pipe);
	return size;
}

/**
 * @brief Check if metadata exists for cache
 */
int cache_metadata_exists(const char *cache_path)
{
	if (!cache_path) {
		return -1;
	}
	
	char metadata_path[4096];
	int ret = cache_metadata_get_path(cache_path, metadata_path, sizeof(metadata_path));
	if (ret != METADATA_SUCCESS) {
		return -1;
	}
	
	return access(metadata_path, F_OK) == 0 ? 1 : 0;
}

/**
 * @brief Increment reference count
 */
int cache_metadata_increment_ref(const char *cache_path)
{
	if (!cache_path) {
		return METADATA_ERROR_INVALID;
	}
	
	struct cache_metadata metadata;
	int ret = cache_metadata_load(cache_path, &metadata);
	if (ret != METADATA_SUCCESS) {
		return ret;
	}
	
	metadata.ref_count++;
	metadata.last_access_time = time(NULL);
	ret = cache_metadata_save(cache_path, &metadata);
	
	/* Clean up stack-allocated metadata strings */
	free(metadata.original_url);
	free(metadata.fork_url);
	free(metadata.owner);
	free(metadata.name);
	free(metadata.fork_organization);
	free(metadata.default_branch);
	
	return ret;
}

/**
 * @brief Decrement reference count
 */
int cache_metadata_decrement_ref(const char *cache_path)
{
	if (!cache_path) {
		return METADATA_ERROR_INVALID;
	}
	
	struct cache_metadata metadata;
	int ret = cache_metadata_load(cache_path, &metadata);
	if (ret != METADATA_SUCCESS) {
		return ret;
	}
	
	if (metadata.ref_count > 0) {
		metadata.ref_count--;
	}
	ret = cache_metadata_save(cache_path, &metadata);
	
	/* Clean up stack-allocated metadata strings */
	free(metadata.original_url);
	free(metadata.fork_url);
	free(metadata.owner);
	free(metadata.name);
	free(metadata.fork_organization);
	free(metadata.default_branch);
	
	return ret;
}

/**
 * @brief Get repository metadata by URL
 */
int cache_metadata_get_by_url(const struct cache_config *config, const char *url, 
	                         struct cache_metadata *metadata)
{
	if (!config || !url || !metadata) {
		return METADATA_ERROR_INVALID;
	}
	
	/* Parse URL to get cache path */
	struct repo_info repo;
	memset(&repo, 0, sizeof(repo));
	
	int ret = repo_info_parse_url(url, &repo);
	if (ret != CACHE_SUCCESS) {
		return METADATA_ERROR_INVALID;
	}
	
	ret = cache_metadata_load(repo.cache_path, metadata);
	repo_info_destroy(&repo);
	
	return ret;
}

/**
 * @brief List all cached repositories with metadata
 */
int cache_metadata_list_all(const struct cache_config *config,
	                       int (*callback)(const struct cache_metadata *metadata, void *user_data),
	                       void *user_data)
{
	if (!config || !callback) {
		return METADATA_ERROR_INVALID;
	}
	
	/* Walk through cache directory */
	char github_path[4096];
	snprintf(github_path, sizeof(github_path), "%s/github.com", config->cache_root);
	
	DIR *github_dir = opendir(github_path);
	if (!github_dir) {
		return 0;
	}
	
	int count = 0;
	const struct dirent *owner_entry;
	while ((owner_entry = readdir(github_dir)) != NULL) {
		if (strcmp(owner_entry->d_name, ".") == 0 || strcmp(owner_entry->d_name, "..") == 0) {
			continue;
		}
		
		char owner_path[4096];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
		snprintf(owner_path, sizeof(owner_path), "%s/%s", github_path, owner_entry->d_name);
#pragma GCC diagnostic pop
		
		DIR *owner_dir = opendir(owner_path);
		if (!owner_dir) {
			continue;
		}
		
		const struct dirent *repo_entry;
		while ((repo_entry = readdir(owner_dir)) != NULL) {
			if (strcmp(repo_entry->d_name, ".") == 0 || strcmp(repo_entry->d_name, "..") == 0) {
				continue;
			}
			
			char repo_path[4096];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
			snprintf(repo_path, sizeof(repo_path), "%s/%s", owner_path, repo_entry->d_name);
#pragma GCC diagnostic pop
			
			/* Check if metadata exists */
			if (cache_metadata_exists(repo_path)) {
				struct cache_metadata metadata;
				if (cache_metadata_load(repo_path, &metadata) == METADATA_SUCCESS) {
					int ret = callback(&metadata, user_data);
					
					/* Clean up loaded metadata strings */
					free(metadata.original_url);
					free(metadata.fork_url);
					free(metadata.owner);
					free(metadata.name);
					free(metadata.fork_organization);
					free(metadata.default_branch);
					
					if (ret != 0) {
						closedir(owner_dir);
						closedir(github_dir);
						return count;
					}
					count++;
				}
			}
		}
		closedir(owner_dir);
	}
	closedir(github_dir);
	
	return count;
}