#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "github_api.h"

/* HTTP response callback for libcurl */
static size_t github_response_callback(void *contents, size_t size, size_t nmemb, struct github_response *response)
{
    size_t total_size = size * nmemb;
    size_t new_size = response->size + total_size;
    
    char *new_data = realloc(response->data, new_size + 1);
    if (!new_data) {
        return 0;
    }
    
    response->data = new_data;
    memcpy(response->data + response->size, contents, total_size);
    response->size = new_size;
    response->data[response->size] = '\0';
    
    return total_size;
}

/* Create GitHub API client */
struct github_client* github_client_create(const char *token)
{
    if (!token || strlen(token) == 0) {
        return NULL;
    }
    
    struct github_client *client = malloc(sizeof(struct github_client));
    if (!client) {
        return NULL;
    }
    
    client->token = malloc(strlen(token) + 1);
    if (!client->token) {
        free(client);
        return NULL;
    }
    strcpy(client->token, token);
    
    client->user_agent = malloc(64);
    if (!client->user_agent) {
        free(client->token);
        free(client);
        return NULL;
    }
    strcpy(client->user_agent, "git-cache/1.0");
    
    client->timeout = 30;
    
    return client;
}

/* Destroy GitHub API client */
void github_client_destroy(struct github_client *client)
{
    if (!client) {
        return;
    }
    
    free(client->token);
    free(client->user_agent);
    free(client);
}

/* Set client timeout */
int github_client_set_timeout(struct github_client *client, int timeout_seconds)
{
    if (!client || timeout_seconds < 1) {
        return GITHUB_ERROR_INVALID;
    }
    
    client->timeout = timeout_seconds;
    return GITHUB_SUCCESS;
}

/* Create GitHub response structure */
struct github_response* github_response_create(void)
{
    struct github_response *response = malloc(sizeof(struct github_response));
    if (!response) {
        return NULL;
    }
    
    response->data = malloc(1);
    if (!response->data) {
        free(response);
        return NULL;
    }
    
    response->data[0] = '\0';
    response->size = 0;
    response->status_code = 0;
    response->error_message = NULL;
    
    return response;
}

/* Destroy GitHub response structure */
void github_response_destroy(struct github_response *response)
{
    if (!response) {
        return;
    }
    
    free(response->data);
    free(response->error_message);
    free(response);
}

/* Create GitHub repository structure */
struct github_repo* github_repo_create(void)
{
    struct github_repo *repo = malloc(sizeof(struct github_repo));
    if (!repo) {
        return NULL;
    }
    
    memset(repo, 0, sizeof(struct github_repo));
    return repo;
}

/* Destroy GitHub repository structure */
void github_repo_destroy(struct github_repo *repo)
{
    if (!repo) {
        return;
    }
    
    free(repo->owner);
    free(repo->name);
    free(repo->full_name);
    free(repo->clone_url);
    free(repo->ssh_url);
    free(repo);
}

/* Make HTTP request to GitHub API */
static int github_make_request(struct github_client *client, const char *method, const char *url, 
                              const char *json_data, struct github_response **response)
{
    if (!client || !url || !response) {
        return GITHUB_ERROR_INVALID;
    }
    
    *response = github_response_create();
    if (!*response) {
        return GITHUB_ERROR_MEMORY;
    }
    
    CURL *curl = curl_easy_init();
    if (!curl) {
        github_response_destroy(*response);
        *response = NULL;
        return GITHUB_ERROR_NETWORK;
    }
    
    /* Set up headers */
    struct curl_slist *headers = NULL;
    char auth_header[GITHUB_MAX_TOKEN_LEN + 32];
    snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", client->token);
    
    headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    
    /* Configure curl */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, client->user_agent);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, client->timeout);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, github_response_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, *response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    /* Set HTTP method */
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (json_data) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        }
    } else if (strcmp(method, "PATCH") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        if (json_data) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        }
    }
    
    /* Perform request */
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(*response)->status_code);
    
    /* Cleanup */
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        (*response)->error_message = malloc(256);
        if ((*response)->error_message) {
            snprintf((*response)->error_message, 256, "CURL error: %s", curl_easy_strerror(res));
        }
        return GITHUB_ERROR_NETWORK;
    }
    
    return GITHUB_SUCCESS;
}

/* Parse JSON repository response */
static int github_parse_repo_json(const char *json_str, struct github_repo *repo)
{
    json_object *root = json_tokener_parse(json_str);
    if (!root) {
        return GITHUB_ERROR_JSON;
    }
    
    json_object *obj;
    
    /* Parse owner */
    json_object *owner_obj;
    if (json_object_object_get_ex(root, "owner", &owner_obj)) {
        if (json_object_object_get_ex(owner_obj, "login", &obj)) {
            const char *owner_str = json_object_get_string(obj);
            repo->owner = malloc(strlen(owner_str) + 1);
            if (repo->owner) {
                strcpy(repo->owner, owner_str);
            }
        }
    }
    
    /* Parse name */
    if (json_object_object_get_ex(root, "name", &obj)) {
        const char *name_str = json_object_get_string(obj);
        repo->name = malloc(strlen(name_str) + 1);
        if (repo->name) {
            strcpy(repo->name, name_str);
        }
    }
    
    /* Parse full_name */
    if (json_object_object_get_ex(root, "full_name", &obj)) {
        const char *full_name_str = json_object_get_string(obj);
        repo->full_name = malloc(strlen(full_name_str) + 1);
        if (repo->full_name) {
            strcpy(repo->full_name, full_name_str);
        }
    }
    
    /* Parse clone_url */
    if (json_object_object_get_ex(root, "clone_url", &obj)) {
        const char *clone_url_str = json_object_get_string(obj);
        repo->clone_url = malloc(strlen(clone_url_str) + 1);
        if (repo->clone_url) {
            strcpy(repo->clone_url, clone_url_str);
        }
    }
    
    /* Parse ssh_url */
    if (json_object_object_get_ex(root, "ssh_url", &obj)) {
        const char *ssh_url_str = json_object_get_string(obj);
        repo->ssh_url = malloc(strlen(ssh_url_str) + 1);
        if (repo->ssh_url) {
            strcpy(repo->ssh_url, ssh_url_str);
        }
    }
    
    /* Parse boolean fields */
    if (json_object_object_get_ex(root, "fork", &obj)) {
        repo->is_fork = json_object_get_boolean(obj);
    }
    
    if (json_object_object_get_ex(root, "private", &obj)) {
        repo->is_private = json_object_get_boolean(obj);
    }
    
    if (json_object_object_get_ex(root, "forks_count", &obj)) {
        repo->fork_count = json_object_get_int(obj);
    }
    
    json_object_put(root);
    return GITHUB_SUCCESS;
}

/* Get repository information */
int github_get_repo(struct github_client *client, const char *owner, const char *repo, 
                   struct github_repo **result)
{
    if (!client || !owner || !repo || !result) {
        return GITHUB_ERROR_INVALID;
    }
    
    char url[GITHUB_MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/repos/%s/%s", GITHUB_API_BASE_URL, owner, repo);
    
    struct github_response *response;
    int ret = github_make_request(client, "GET", url, NULL, &response);
    
    if (ret != GITHUB_SUCCESS) {
        return ret;
    }
    
    if (response->status_code == 404) {
        github_response_destroy(response);
        return GITHUB_ERROR_NOT_FOUND;
    }
    
    if (response->status_code == 403) {
        github_response_destroy(response);
        return GITHUB_ERROR_FORBIDDEN;
    }
    
    if (response->status_code != 200) {
        github_response_destroy(response);
        return GITHUB_ERROR_NETWORK;
    }
    
    *result = github_repo_create();
    if (!*result) {
        github_response_destroy(response);
        return GITHUB_ERROR_MEMORY;
    }
    
    ret = github_parse_repo_json(response->data, *result);
    github_response_destroy(response);
    
    if (ret != GITHUB_SUCCESS) {
        github_repo_destroy(*result);
        *result = NULL;
    }
    
    return ret;
}

/* Get error string for error code */
const char* github_get_error_string(int error_code)
{
    switch (error_code) {
        case GITHUB_SUCCESS:
            return "Success";
        case GITHUB_ERROR_MEMORY:
            return "Memory allocation error";
        case GITHUB_ERROR_NETWORK:
            return "Network error";
        case GITHUB_ERROR_AUTH:
            return "Authentication error";
        case GITHUB_ERROR_NOT_FOUND:
            return "Repository not found";
        case GITHUB_ERROR_FORBIDDEN:
            return "Access forbidden";
        case GITHUB_ERROR_JSON:
            return "JSON parsing error";
        case GITHUB_ERROR_INVALID:
            return "Invalid parameters";
        default:
            return "Unknown error";
    }
}