/**
 * @file repo_info_stub.c
 * @brief Stub implementations for testing
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "git-cache.h"

/* Minimal implementation for testing */
int repo_info_parse_url(const char *url, struct repo_info *repo)
{
	if (!url || !repo) {
		return -1;
	}
	
	/* Simple parsing for test purposes */
	repo->original_url = strdup(url);
	
	/* Extract repo name from URL */
	const char *last_slash = strrchr(url, '/');
	if (last_slash) {
		const char *name_start = last_slash + 1;
		char *name = strdup(name_start);
		
		/* Remove .git suffix if present */
		char *git_suffix = strstr(name, ".git");
		if (git_suffix) {
			*git_suffix = '\0';
		}
		
		repo->name = name;
	}
	
	/* Determine type */
	if (strstr(url, "github.com")) {
		repo->type = REPO_TYPE_GITHUB;
	} else {
		repo->type = REPO_TYPE_UNKNOWN;
	}
	
	return 0;
}

void repo_info_destroy(struct repo_info *repo)
{
	if (!repo) {
		return;
	}
	
	free(repo->original_url);
	free(repo->fork_url);
	free(repo->owner);
	free(repo->name);
	free(repo->cache_path);
	free(repo->checkout_path);
	free(repo->modifiable_path);
	free(repo->fork_organization);
	free(repo);
}