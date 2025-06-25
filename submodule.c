/**
 * @file submodule.c
 * @brief Git submodule support for git-cache
 * 
 * This module provides functionality for parsing .gitmodules files and
 * managing submodule caching with the git-cache system.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "git-cache.h"
#include "submodule.h"

/**
 * @brief Parse all submodules from .gitmodules file
 * 
 * Internal implementation that reads the entire file at once
 */
static int parse_gitmodules_internal(FILE *f, struct submodule_list *list)
{
	char line[1024];
	struct submodule_info current_info;
	int in_submodule = 0;
	
	memset(&current_info, 0, sizeof(current_info));
	
	while (fgets(line, sizeof(line), f)) {
		/* Remove trailing newline */
		size_t len = strlen(line);
		if (len > 0 && line[len-1] == '\n') {
			line[len-1] = '\0';
		}
		
		/* Skip empty lines and comments */
		if (line[0] == '\0' || line[0] == '#') {
			continue;
		}
		
		/* Check for submodule section */
		if (line[0] == '[') {
			/* Save previous submodule if complete */
			if (in_submodule && current_info.name[0] != '\0' && 
			    current_info.path[0] != '\0' && current_info.url[0] != '\0') {
				/* Ensure we have space */
				if (list->count >= list->capacity) {
					list->capacity *= 2;
					struct submodule_info *new_array = realloc(list->submodules, 
						list->capacity * sizeof(struct submodule_info));
					if (!new_array) {
						return -1;
					}
					list->submodules = new_array;
				}
				
				/* Add submodule to list */
				memcpy(&list->submodules[list->count], &current_info, sizeof(struct submodule_info));
				list->count++;
			}
			
			/* Reset for new section */
			memset(&current_info, 0, sizeof(current_info));
			in_submodule = 0;
			
			/* Parse [submodule "name"] */
			if (strncmp(line, "[submodule \"", 12) == 0) {
				char *end = strrchr(line, '"');
				if (end && end > line + 12) {
					size_t name_len = end - (line + 12);
					if (name_len >= sizeof(current_info.name)) {
						name_len = sizeof(current_info.name) - 1;
					}
					strncpy(current_info.name, line + 12, name_len);
					current_info.name[name_len] = '\0';
					in_submodule = 1;
				}
			}
		} else if (in_submodule) {
			/* Parse submodule properties */
			char *trimmed = line;
			while (*trimmed == ' ' || *trimmed == '\t') {
				trimmed++;
			}
			
			if (strncmp(trimmed, "path = ", 7) == 0) {
				strncpy(current_info.path, trimmed + 7, sizeof(current_info.path) - 1);
				current_info.path[sizeof(current_info.path) - 1] = '\0';
			} else if (strncmp(trimmed, "url = ", 6) == 0) {
				strncpy(current_info.url, trimmed + 6, sizeof(current_info.url) - 1);
				current_info.url[sizeof(current_info.url) - 1] = '\0';
			} else if (strncmp(trimmed, "branch = ", 9) == 0) {
				strncpy(current_info.branch, trimmed + 9, sizeof(current_info.branch) - 1);
				current_info.branch[sizeof(current_info.branch) - 1] = '\0';
			}
		}
	}
	
	/* Save last submodule if complete */
	if (in_submodule && current_info.name[0] != '\0' && 
	    current_info.path[0] != '\0' && current_info.url[0] != '\0') {
		/* Ensure we have space */
		if (list->count >= list->capacity) {
			list->capacity *= 2;
			struct submodule_info *new_array = realloc(list->submodules, 
				list->capacity * sizeof(struct submodule_info));
			if (!new_array) {
				return -1;
			}
			list->submodules = new_array;
		}
		
		/* Add submodule to list */
		memcpy(&list->submodules[list->count], &current_info, sizeof(struct submodule_info));
		list->count++;
	}
	
	return 0;
}

int parse_gitmodules(const char *gitmodules_path, struct submodule_list *list)
{
	FILE *f = fopen(gitmodules_path, "r");
	if (!f) {
		if (errno == ENOENT) {
			/* No .gitmodules file, not an error */
			list->count = 0;
			list->submodules = NULL;
			return 0;
		}
		return -1;
	}
	
	/* Initialize list */
	list->count = 0;
	list->capacity = 4;
	list->submodules = malloc(list->capacity * sizeof(struct submodule_info));
	if (!list->submodules) {
		fclose(f);
		return -1;
	}
	
	/* Parse all submodules */
	int ret = parse_gitmodules_internal(f, list);
	if (ret != 0) {
		free(list->submodules);
		fclose(f);
		return -1;
	}
	
	fclose(f);
	return 0;
}

void free_submodule_list(struct submodule_list *list)
{
	if (list->submodules) {
		free(list->submodules);
		list->submodules = NULL;
	}
	list->count = 0;
	list->capacity = 0;
}

int process_submodules(struct repo_info *repo, struct cache_config *config, int recursive)
{
	char gitmodules_path[4096];
	struct submodule_list submodules;
	int ret = 0;
	
	/* TODO: Implement recursive submodule processing */
	(void)recursive; /* Suppress unused parameter warning */
	
	/* Construct path to .gitmodules in the checkout */
	snprintf(gitmodules_path, sizeof(gitmodules_path), "%s/.gitmodules", repo->checkout_path);
	
	/* Parse .gitmodules file */
	if (parse_gitmodules(gitmodules_path, &submodules) != 0) {
		return -1;
	}
	
	if (config->verbose && submodules.count > 0) {
		printf("Found %zu submodule(s) in %s\n", submodules.count, repo->name);
	}
	
	/* Process each submodule */
	for (size_t i = 0; i < submodules.count; i++) {
		struct submodule_info *sub = &submodules.submodules[i];
		
		if (config->verbose) {
			printf("  Submodule '%s' at path '%s' from %s\n", 
				sub->name, sub->path, sub->url);
		}
		
		/* Create cache entry for submodule */
		if (cache_submodule(repo, sub, config) != 0) {
			fprintf(stderr, "Failed to cache submodule '%s'\n", sub->name);
			ret = -1;
			/* Continue with other submodules */
		}
		
		/* Initialize submodule in checkout */
		if (init_submodule_checkout(repo, sub, config) != 0) {
			fprintf(stderr, "Failed to initialize submodule '%s' checkout\n", sub->name);
			ret = -1;
		}
	}
	
	free_submodule_list(&submodules);
	return ret;
}

int cache_submodule(struct repo_info *parent_repo, struct submodule_info *sub, 
                    struct cache_config *config)
{
	/* Create a repo_info for the submodule */
	struct repo_info *sub_repo = malloc(sizeof(struct repo_info));
	if (!sub_repo) {
		return -1;
	}
	
	memset(sub_repo, 0, sizeof(struct repo_info));
	sub_repo->original_url = strdup(sub->url);
	sub_repo->strategy = parent_repo->strategy;
	
	/* Parse the submodule URL to determine type and extract owner/name */
	if (repo_info_parse_url(sub->url, sub_repo) != 0) {
		repo_info_destroy(sub_repo);
		return -1;
	}
	
	/* Setup paths for submodule cache */
	char submodule_cache_base[4096];
	snprintf(submodule_cache_base, sizeof(submodule_cache_base), 
		"%s/submodules/%s", parent_repo->cache_path, sub->path);
	
	/* Create submodule cache directory if needed */
	char mkdir_cmd[4096];
	snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p $(dirname %s)", submodule_cache_base);
	if (system(mkdir_cmd) != 0) {
		repo_info_destroy(sub_repo);
		return -1;
	}
	
	/* Set the cache path for the submodule */
	free(sub_repo->cache_path);
	sub_repo->cache_path = strdup(submodule_cache_base);
	
	/* Clone or update the submodule cache */
	if (access(sub_repo->cache_path, F_OK) != 0) {
		/* Clone the submodule into cache */
		if (config->verbose) {
			printf("    Cloning submodule '%s' into cache...\n", sub->name);
		}
		
		char clone_cmd[8192];
		snprintf(clone_cmd, sizeof(clone_cmd), 
			"git clone --bare %s %s %s", 
			config->verbose ? "" : "-q",
			sub->url, 
			sub_repo->cache_path);
		
		if (system(clone_cmd) != 0) {
			fprintf(stderr, "Failed to clone submodule '%s'\n", sub->name);
			repo_info_destroy(sub_repo);
			return -1;
		}
	} else {
		/* Update existing submodule cache */
		if (config->verbose) {
			printf("    Updating submodule '%s' cache...\n", sub->name);
		}
		
		char fetch_cmd[8192];
		snprintf(fetch_cmd, sizeof(fetch_cmd),
			"cd %s && git fetch %s --all --prune",
			sub_repo->cache_path,
			config->verbose ? "" : "-q");
		
		if (system(fetch_cmd) != 0) {
			fprintf(stderr, "Warning: Failed to update submodule '%s' cache\n", sub->name);
			/* Continue anyway */
		}
	}
	
	repo_info_destroy(sub_repo);
	return 0;
}

int init_submodule_checkout(struct repo_info *parent_repo, struct submodule_info *sub,
                           struct cache_config *config)
{
	char submodule_path[4096];
	char submodule_cache[4096];
	char reference_flag[4096];
	
	/* Construct full path to submodule in checkout */
	snprintf(submodule_path, sizeof(submodule_path), 
		"%s/%s", parent_repo->checkout_path, sub->path);
	
	/* Construct path to submodule cache */
	snprintf(submodule_cache, sizeof(submodule_cache),
		"%s/submodules/%s", parent_repo->cache_path, sub->path);
	
	/* Check if submodule directory exists */
	struct stat st;
	if (stat(submodule_path, &st) == 0) {
		if (config->verbose) {
			printf("    Submodule '%s' already initialized\n", sub->name);
		}
		return 0;
	}
	
	/* Initialize submodule with reference to cache */
	if (config->verbose) {
		printf("    Initializing submodule '%s' with reference to cache...\n", sub->name);
	}
	
	/* Use git submodule update with reference */
	snprintf(reference_flag, sizeof(reference_flag), "--reference=%s", submodule_cache);
	
	char init_cmd[8192];
	snprintf(init_cmd, sizeof(init_cmd),
		"cd %s && git submodule update --init %s %s -- %s",
		parent_repo->checkout_path,
		config->verbose ? "" : "-q",
		reference_flag,
		sub->path);
	
	if (system(init_cmd) != 0) {
		fprintf(stderr, "Failed to initialize submodule '%s'\n", sub->name);
		return -1;
	}
	
	return 0;
}