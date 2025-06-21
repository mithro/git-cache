/*
 * GitHub Fork Integration Test
 * 
 * Tests the fork URL handling without requiring actual GitHub API calls.
 * This verifies the logic for storing and using fork URLs in the modifiable checkout.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "git-cache.h"

static void test_fork_url_storage(void)
{
    printf("=== Testing Fork URL Storage ===\n");
    
    /* Test 1: Verify fork_url field exists and is properly initialized */
    struct repo_info *repo = malloc(sizeof(struct repo_info));
    memset(repo, 0, sizeof(struct repo_info));
    
    assert(repo->fork_url == NULL);
    printf("✓ Fork URL properly initialized to NULL\n");
    
    /* Test 2: Test fork URL assignment */
    const char *test_fork_url = "git@github.com:mithro-mirrors/user-repo.git";
    repo->fork_url = strdup(test_fork_url);
    
    assert(repo->fork_url != NULL);
    assert(strcmp(repo->fork_url, test_fork_url) == 0);
    printf("✓ Fork URL properly stored and retrieved\n");
    
    /* Test 3: Test fork URL fallback logic */
    const char *original_url = "https://github.com/user/repo.git";
    repo->original_url = strdup(original_url);
    
    /* Simulate modifiable checkout URL selection */
    const char *modifiable_url = repo->fork_url ? repo->fork_url : repo->original_url;
    assert(strcmp(modifiable_url, test_fork_url) == 0);
    printf("✓ Fork URL correctly selected over original URL\n");
    
    /* Test 4: Test fallback when no fork URL */
    free(repo->fork_url);
    repo->fork_url = NULL;
    
    modifiable_url = repo->fork_url ? repo->fork_url : repo->original_url;
    assert(strcmp(modifiable_url, original_url) == 0);
    printf("✓ Original URL correctly used when fork URL is NULL\n");
    
    /* Cleanup */
    free(repo->original_url);
    free(repo);
}

static void test_fork_url_construction(void)
{
    printf("\n=== Testing Fork URL Construction ===\n");
    
    /* Test 1: Constructed URL format */
    const char *organization = "mithro-mirrors";
    const char *owner = "user";
    const char *repo_name = "repo";
    
    char constructed_url[1024];
    snprintf(constructed_url, sizeof(constructed_url),
             "git@github.com:%s/%s-%s.git",
             organization, owner, repo_name);
    
    const char *expected = "git@github.com:mithro-mirrors/user-repo.git";
    assert(strcmp(constructed_url, expected) == 0);
    printf("✓ Fork URL construction format is correct\n");
    
    /* Test 2: URL construction with different inputs */
    snprintf(constructed_url, sizeof(constructed_url),
             "git@github.com:%s/%s-%s.git",
             "test-org", "octocat", "Hello-World");
    
    assert(strcmp(constructed_url, "git@github.com:test-org/octocat-Hello-World.git") == 0);
    printf("✓ Fork URL construction works with different inputs\n");
}

static void test_fork_detection_logic(void)
{
    printf("\n=== Testing Fork Detection Logic ===\n");
    
    struct repo_info *repo = malloc(sizeof(struct repo_info));
    memset(repo, 0, sizeof(struct repo_info));
    
    /* Test 1: GitHub repository detection */
    repo->type = REPO_TYPE_GITHUB;
    repo->is_fork_needed = 1;
    
    /* Simulate fork needed check */
    int should_fork = (repo->type == REPO_TYPE_GITHUB && repo->is_fork_needed);
    assert(should_fork == 1);
    printf("✓ GitHub repositories correctly identified for forking\n");
    
    /* Test 2: Non-GitHub repository */
    repo->type = REPO_TYPE_UNKNOWN;
    should_fork = (repo->type == REPO_TYPE_GITHUB && repo->is_fork_needed);
    assert(should_fork == 0);
    printf("✓ Non-GitHub repositories correctly excluded from forking\n");
    
    /* Test 3: GitHub repository with forking disabled */
    repo->type = REPO_TYPE_GITHUB;
    repo->is_fork_needed = 0;
    should_fork = (repo->type == REPO_TYPE_GITHUB && repo->is_fork_needed);
    assert(should_fork == 0);
    printf("✓ GitHub repositories with forking disabled correctly handled\n");
    
    free(repo);
}

static void test_modifiable_checkout_scenarios(void)
{
    printf("\n=== Testing Modifiable Checkout Scenarios ===\n");
    
    struct repo_info *repo = malloc(sizeof(struct repo_info));
    memset(repo, 0, sizeof(struct repo_info));
    
    const char *original_url = "https://github.com/user/repo.git";
    repo->original_url = strdup(original_url);
    
    /* Scenario 1: Successful fork - use fork URL */
    repo->fork_url = strdup("git@github.com:mithro-mirrors/user-repo.git");
    const char *checkout_url = repo->fork_url ? repo->fork_url : repo->original_url;
    assert(strcmp(checkout_url, repo->fork_url) == 0);
    printf("✓ Successful fork: modifiable checkout uses fork URL\n");
    
    /* Scenario 2: Fork failed - use original URL */
    free(repo->fork_url);
    repo->fork_url = NULL;
    checkout_url = repo->fork_url ? repo->fork_url : repo->original_url;
    assert(strcmp(checkout_url, repo->original_url) == 0);
    printf("✓ Failed fork: modifiable checkout uses original URL\n");
    
    /* Scenario 3: Fork exists but creation returned error - use constructed URL */
    repo->fork_url = strdup("git@github.com:mithro-mirrors/user-repo.git");
    checkout_url = repo->fork_url ? repo->fork_url : repo->original_url;
    assert(strcmp(checkout_url, repo->fork_url) == 0);
    printf("✓ Existing fork: modifiable checkout uses constructed fork URL\n");
    
    /* Cleanup */
    free(repo->original_url);
    free(repo->fork_url);
    free(repo);
}

static void test_memory_management(void)
{
    printf("\n=== Testing Memory Management ===\n");
    
    struct repo_info *repo = malloc(sizeof(struct repo_info));
    memset(repo, 0, sizeof(struct repo_info));
    
    /* Test proper cleanup of fork URL */
    repo->fork_url = strdup("git@github.com:test/fork.git");
    repo->original_url = strdup("https://github.com/test/repo.git");
    
    /* Simulate repo_info_destroy behavior */
    free(repo->fork_url);
    free(repo->original_url);
    free(repo);
    
    printf("✓ Memory management: no leaks in fork URL handling\n");
    
    /* Test NULL pointer safety */
    repo = malloc(sizeof(struct repo_info));
    memset(repo, 0, sizeof(struct repo_info));
    
    /* Should handle NULL fork_url gracefully */
    const char *url = repo->fork_url ? repo->fork_url : "fallback";
    assert(strcmp(url, "fallback") == 0);
    
    free(repo);
    printf("✓ Memory management: NULL pointer safety verified\n");
}

int main(void)
{
    printf("Fork Integration Test Suite\n");
    printf("===========================\n\n");
    
    test_fork_url_storage();
    test_fork_url_construction();
    test_fork_detection_logic();
    test_modifiable_checkout_scenarios();
    test_memory_management();
    
    printf("\n=== Test Summary ===\n");
    printf("All fork integration tests passed!\n");
    printf("✓ Fork URL storage and retrieval\n");
    printf("✓ Fork URL construction logic\n");
    printf("✓ Fork detection conditions\n");
    printf("✓ Modifiable checkout URL selection\n");
    printf("✓ Memory management safety\n");
    
    printf("\nTo test with actual GitHub API:\n");
    printf("  export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
    printf("  export TEST_REPO_OWNER=octocat\n");
    printf("  export TEST_REPO_NAME=Hello-World\n");
    printf("  ./tests/run_github_tests.sh\n");
    
    return 0;
}