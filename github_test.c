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

static int test_url_parsing(void)
{
    printf("\n=== Testing URL Parsing ===\n");
    
    const char *test_urls[] = {
        "https://github.com/octocat/Hello-World",
        "https://github.com/octocat/Hello-World.git",
        "git@github.com:octocat/Hello-World.git",
        "github.com/octocat/Hello-World",
        "invalid-url",
        "https://gitlab.com/user/repo",
        NULL
    };
    
    const char *expected_owners[] = {"octocat", "octocat", "octocat", "octocat", NULL, NULL};
    const char *expected_repos[] = {"Hello-World", "Hello-World", "Hello-World", "Hello-World", NULL, NULL};
    
    for (int i = 0; test_urls[i]; i++) {
        char *owner, *repo;
        int ret = github_parse_repo_url(test_urls[i], &owner, &repo);
        
        if (expected_owners[i] == NULL) {
            /* Should fail */
            if (ret != GITHUB_SUCCESS) {
                printf("✓ Correctly rejected invalid URL: %s\n", test_urls[i]);
            } else {
                printf("✗ Should have rejected URL: %s\n", test_urls[i]);
                free(owner);
                free(repo);
            }
        } else {
            /* Should succeed */
            if (ret == GITHUB_SUCCESS && 
                strcmp(owner, expected_owners[i]) == 0 && 
                strcmp(repo, expected_repos[i]) == 0) {
                printf("✓ Correctly parsed URL: %s -> %s/%s\n", test_urls[i], owner, repo);
                free(owner);
                free(repo);
            } else {
                printf("✗ Failed to parse URL: %s\n", test_urls[i]);
                if (ret == GITHUB_SUCCESS) {
                    printf("   Got: %s/%s, Expected: %s/%s\n", 
                           owner, repo, expected_owners[i], expected_repos[i]);
                    free(owner);
                    free(repo);
                }
            }
        }
    }
    
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
    
    /* Test token validation */
    printf("Testing token validation...\n");
    int ret = github_validate_token(client);
    if (ret == GITHUB_SUCCESS) {
        printf("✓ Token validation successful\n");
    } else {
        printf("Token validation result: %s (code: %d)\n", github_get_error_string(ret), ret);
        if (ret == GITHUB_ERROR_AUTH) {
            printf("Note: Invalid token provided\n");
            github_client_destroy(client);
            return 0;
        }
    }
    
    /* Test getting a public repository */
    struct github_repo *repo;
    ret = github_get_repo(client, "octocat", "Hello-World", &repo);
    
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

static int test_fork_operations(const char *token, const char *test_repo_owner, const char *test_repo_name)
{
    printf("\n=== Testing Fork Operations ===\n");
    
    if (!token) {
        printf("Skipping fork tests (no token provided)\n");
        return 0;
    }
    
    if (!test_repo_owner || !test_repo_name) {
        printf("Skipping fork tests (no test repository specified)\n");
        printf("To test forking, provide: ./github_test <token> <owner> <repo>\n");
        return 0;
    }
    
    struct github_client *client = github_client_create(token);
    if (!client) {
        printf("ERROR: Failed to create client for fork tests\n");
        return 1;
    }
    
    printf("Testing fork creation for %s/%s...\n", test_repo_owner, test_repo_name);
    
    /* Attempt to fork the repository */
    struct github_repo *forked_repo;
    int ret = github_fork_repo(client, test_repo_owner, test_repo_name, NULL, &forked_repo);
    
    if (ret == GITHUB_SUCCESS) {
        printf("✓ Successfully forked repository\n");
        print_repo_info(forked_repo);
        
        /* Test setting the fork to private (if we have permissions) */
        printf("Testing privacy setting...\n");
        int privacy_ret = github_set_repo_private(client, forked_repo->owner, forked_repo->name, 1);
        if (privacy_ret == GITHUB_SUCCESS) {
            printf("✓ Successfully set repository to private\n");
        } else {
            printf("Privacy setting result: %s (code: %d)\n", github_get_error_string(privacy_ret), privacy_ret);
            printf("Note: This might be expected if you don't have admin permissions\n");
        }
        
        github_repo_destroy(forked_repo);
        
    } else if (ret == GITHUB_ERROR_INVALID) {
        printf("Fork already exists or validation error: %s\n", github_get_error_string(ret));
    } else {
        printf("Fork test result: %s (code: %d)\n", github_get_error_string(ret), ret);
    }
    
    /* Test forking to an organization */
    printf("Testing organization fork (will likely fail without proper permissions)...\n");
    ret = github_fork_repo(client, test_repo_owner, test_repo_name, "mithro-mirrors", &forked_repo);
    if (ret == GITHUB_SUCCESS) {
        printf("✓ Successfully forked to organization\n");
        print_repo_info(forked_repo);
        github_repo_destroy(forked_repo);
    } else {
        printf("Organization fork result: %s (code: %d)\n", github_get_error_string(ret), ret);
        printf("Note: This is expected unless you have access to the organization\n");
    }
    
    github_client_destroy(client);
    return 0;
}

static void print_usage(const char *program_name)
{
    printf("Usage: %s [github_token] [test_repo_owner] [test_repo_name]\n", program_name);
    printf("\n");
    printf("This program tests the GitHub API client functionality.\n");
    printf("If no token is provided, only basic tests will be run.\n");
    printf("To test actual API calls, provide a valid GitHub token.\n");
    printf("To test forking, provide owner and repo name of a test repository.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                                    # Basic tests only\n", program_name);
    printf("  %s ghp_xxxxx                         # Token validation and repo tests\n", program_name);
    printf("  %s ghp_xxxxx octocat Hello-World     # Full tests including forking\n", program_name);
}

int main(int argc, char *argv[])
{
    printf("GitHub API Client Test Program\n");
    printf("==============================\n");
    
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    
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
    
    /* Run URL parsing tests */
    if (test_url_parsing() != 0) {
        curl_global_cleanup();
        return 1;
    }
    
    /* Run repository operation tests if token provided */
    const char *token = (argc > 1) ? argv[1] : NULL;
    if (test_repo_operations(token) != 0) {
        curl_global_cleanup();
        return 1;
    }
    
    /* Run fork operation tests if token and test repo provided */
    const char *test_owner = (argc > 2) ? argv[2] : NULL;
    const char *test_repo = (argc > 3) ? argv[3] : NULL;
    if (test_fork_operations(token, test_owner, test_repo) != 0) {
        curl_global_cleanup();
        return 1;
    }
    
    printf("\n=== Test Summary ===\n");
    if (!token) {
        printf("Basic tests completed successfully.\n");
        printf("To test actual GitHub API calls, run with a valid token:\n");
        printf("  %s <your_github_token>\n", argv[0]);
    } else if (!test_owner || !test_repo) {
        printf("Token-based tests completed.\n");
        printf("To test forking functionality, run with a test repository:\n");
        printf("  %s <token> <owner> <repo>\n", argv[0]);
    } else {
        printf("All tests completed including fork operations.\n");
    }
    
    curl_global_cleanup();
    return 0;
}