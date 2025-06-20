#ifndef GITHUB_API_H
#define GITHUB_API_H

#include <stddef.h>

#define GITHUB_API_BASE_URL "https://api.github.com"
#define GITHUB_MAX_URL_LEN 512
#define GITHUB_MAX_TOKEN_LEN 256
#define GITHUB_MAX_RESPONSE_LEN 65536

/* GitHub API response structure */
struct github_response {
	char *data;
	size_t size;
	long status_code;
	char *error_message;
};

/* GitHub repository information */
struct github_repo {
	char *owner;
	char *name;
	char *full_name;
	char *clone_url;
	char *ssh_url;
	int is_fork;
	int is_private;
	int fork_count;
};

/* GitHub API client */
struct github_client {
	char *token;
	char *user_agent;
	int timeout;
};

/* Function prototypes */

/* Client management */
struct github_client* github_client_create(const char *token);
void github_client_destroy(struct github_client *client);
int github_client_set_timeout(struct github_client *client, int timeout_seconds);

/* HTTP response management */
struct github_response* github_response_create(void);
void github_response_destroy(struct github_response *response);

/* Repository operations */
int github_get_repo(struct github_client *client, const char *owner, const char *repo, 
	               struct github_repo **result);
int github_fork_repo(struct github_client *client, const char *owner, const char *repo, 
	                 const char *organization, struct github_repo **result);
int github_set_repo_private(struct github_client *client, const char *owner, const char *repo, 
	                       int is_private);

/* Repository structure management */
struct github_repo* github_repo_create(void);
void github_repo_destroy(struct github_repo *repo);

/* Utility functions */
int github_parse_repo_url(const char *url, char **owner, char **repo);
int github_validate_token(struct github_client *client);

/* Error handling */
const char* github_get_error_string(int error_code);

/* Error codes */
#define GITHUB_SUCCESS          0
#define GITHUB_ERROR_MEMORY     -1
#define GITHUB_ERROR_NETWORK    -2
#define GITHUB_ERROR_AUTH       -3
#define GITHUB_ERROR_NOT_FOUND  -4
#define GITHUB_ERROR_FORBIDDEN  -5
#define GITHUB_ERROR_JSON       -6
#define GITHUB_ERROR_INVALID    -7

#endif /* GITHUB_API_H */