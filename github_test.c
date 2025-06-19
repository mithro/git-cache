#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "github_api.h"

static void print_repo_info(const struct github_repo *repo)
{
    if (!repo) {
        printf("Repository is NULL\n");
        return;
    }
    
    printf("Repository Information:\n");
    printf("  Owner: %s\n", repo->owner ? repo->owner : "N/A");
    printf("  Name: %s\n", repo->name ? repo->name : "N/A");
    printf("  Full Name: %s\n", repo->full_name ? repo->full_name : "N/A");
    printf("  Clone URL: %s\n", repo->clone_url ? repo->clone_url : "N/A");
    printf("  SSH URL: %s\n", repo->ssh_url ? repo->ssh_url : "N/A");
    printf("  Is Fork: %s\n", repo->is_fork ? "Yes" : "No");
    printf("  Is Private: %s\n", repo->is_private ? "Yes" : "No");
    printf("  Fork Count: %d\n", repo->fork_count);
}

static int test_basic_functionality(void)
{
    printf("=== Testing Basic Functionality ===\n");
    
    /* Test client creation with invalid token */
    struct github_client *client = github_client_create(NULL);
    if (client != NULL) {
        printf("ERROR: Client creation should fail with NULL token\n");
        return 1;
    }
    printf("✓ NULL token handling works\n");
    
    /* Test client creation with empty token */
    client = github_client_create("");
    if (client != NULL) {
        printf("ERROR: Client creation should fail with empty token\n");
        return 1;
    }
    printf("✓ Empty token handling works\n");
    
    /* Test client creation with dummy token */
    client = github_client_create("dummy_token_for_testing");
    if (client == NULL) {
        printf("ERROR: Client creation failed with valid token\n");
        return 1;
    }
    printf("✓ Client creation works\n");
    
    /* Test timeout setting */
    int ret = github_client_set_timeout(client, 10);
    if (ret != GITHUB_SUCCESS) {
        printf("ERROR: Failed to set timeout\n");
        github_client_destroy(client);
        return 1;
    }
    printf("✓ Timeout setting works\n");
    
    /* Test invalid timeout */
    ret = github_client_set_timeout(client, 0);
    if (ret == GITHUB_SUCCESS) {
        printf("ERROR: Should fail with invalid timeout\n");
        github_client_destroy(client);
        return 1;
    }
    printf("✓ Invalid timeout handling works\n");
    
    github_client_destroy(client);
    printf("✓ Client destruction works\n");
    
    return 0;
}

static int test_repo_operations(const char *token)
{
    printf("\n=== Testing Repository Operations ===\n");
    
    if (!token) {
        printf("Skipping repository tests (no token provided)\n");
        return 0;
    }
    
    struct github_client *client = github_client_create(token);
    if (!client) {
        printf("ERROR: Failed to create client for repo tests\n");
        return 1;
    }
    
    /* Test getting a public repository */
    struct github_repo *repo;
    int ret = github_get_repo(client, "octocat", "Hello-World", &repo);
    
    if (ret == GITHUB_SUCCESS) {
        printf("✓ Successfully retrieved public repository\n");
        print_repo_info(repo);
        github_repo_destroy(repo);
    } else {
        printf("Repository test result: %s (code: %d)\n", github_get_error_string(ret), ret);
        printf("Note: This might be expected if using a dummy token\n");
    }
    
    /* Test getting a non-existent repository */
    ret = github_get_repo(client, "nonexistent", "repo", &repo);
    if (ret == GITHUB_ERROR_NOT_FOUND) {
        printf("✓ Correctly handled non-existent repository\n");
    } else {
        printf("Non-existent repo test result: %s (code: %d)\n", github_get_error_string(ret), ret);
    }
    
    github_client_destroy(client);
    return 0;
}

static void print_usage(const char *program_name)
{
    printf("Usage: %s [github_token]\n", program_name);
    printf("\n");
    printf("This program tests the GitHub API client functionality.\n");
    printf("If no token is provided, only basic tests will be run.\n");
    printf("To test actual API calls, provide a valid GitHub token.\n");
    printf("\n");
    printf("Example:\n");
    printf("  %s ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n", program_name);
}

int main(int argc, char *argv[])
{
    printf("GitHub API Client Test Program\n");
    printf("==============================\n");
    
    /* Initialize curl globally */
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        printf("ERROR: Failed to initialize libcurl\n");
        return 1;
    }
    
    /* Run basic functionality tests */
    if (test_basic_functionality() != 0) {
        curl_global_cleanup();
        return 1;
    }
    
    /* Run repository operation tests if token provided */
    const char *token = (argc > 1) ? argv[1] : NULL;
    if (test_repo_operations(token) != 0) {
        curl_global_cleanup();
        return 1;
    }
    
    if (!token) {
        printf("\n=== Test Summary ===\n");
        printf("Basic tests completed successfully.\n");
        printf("To test actual GitHub API calls, run with a valid token:\n");
        printf("  %s <your_github_token>\n", argv[0]);
    } else {
        printf("\n=== Test Summary ===\n");
        printf("All tests completed.\n");
    }
    
    curl_global_cleanup();
    return 0;
}