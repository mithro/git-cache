/**
 * @file metadata_test_stub.c
 * @brief Stub implementations for metadata testing
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "git-cache.h"

/**
 * @brief Simple repo_info_parse_url stub for testing
 */
int repo_info_parse_url(const char *url, struct repo_info *repo)
{
	if (!url || !repo) {
		return CACHE_ERROR_ARGS;
	}
	
	/* Initialize repo structure */
	memset(repo, 0, sizeof(*repo));
	
	/* Simple GitHub URL parsing for testing */
	if (strstr(url, "github.com")) {
		repo->type = REPO_TYPE_GITHUB;
		repo->original_url = strdup(url);
		
		/* Extract owner and name from URL like github.com/owner/name.git */
		char *url_copy = strdup(url);
		char *path_start = strstr(url_copy, "github.com/");
		if (path_start) {
			path_start += strlen("github.com/");
			char *owner_start = path_start;
			char *slash = strchr(owner_start, '/');
			if (slash) {
				*slash = '\0';
				repo->owner = strdup(owner_start);
				
				char *name_start = slash + 1;
				char *dot_git = strstr(name_start, ".git");
				if (dot_git) {
					*dot_git = '\0';
				}
				repo->name = strdup(name_start);
			}
		}
		free(url_copy);
		
		/* Set default paths */
		if (repo->owner && repo->name) {
			char path_buf[1024];
			snprintf(path_buf, sizeof(path_buf), "/tmp/test_cache/github.com/%s/%s", repo->owner, repo->name);
			repo->cache_path = strdup(path_buf);
		}
		
		return CACHE_SUCCESS;
	}
	
	return CACHE_ERROR_ARGS;
}

/**
 * @brief Simple repo_info_destroy stub for testing
 */
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
	
	memset(repo, 0, sizeof(*repo));
}