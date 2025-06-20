#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "github_api.h"

struct url_test {
	const char *url;
	const char *expected_owner;
	const char *expected_repo;
	int should_succeed;
};

static struct url_test url_tests[] = {
	/* Standard HTTPS URLs */
	{"https://github.com/owner/repo", "owner", "repo", 1},
	{"https://github.com/owner/repo.git", "owner", "repo", 1},
	{"https://github.com/owner/repo/", "owner", "repo", 1},
	{"https://github.com/user-name/repo-name.git", "user-name", "repo-name", 1},
	
	/* HTTP URLs */
	{"http://github.com/owner/repo", "owner", "repo", 1},
	{"http://github.com/owner/repo.git", "owner", "repo", 1},
	
	/* Git protocol */
	{"git://github.com/owner/repo.git", "owner", "repo", 1},
	{"git://github.com/owner/repo", "owner", "repo", 1},
	
	/* Git+HTTPS/HTTP */
	{"git+https://github.com/owner/repo.git", "owner", "repo", 1},
	{"git+http://github.com/owner/repo.git", "owner", "repo", 1},
	
	/* SSH URLs - various formats */
	{"git@github.com:owner/repo.git", "owner", "repo", 1},
	{"git@github.com:owner/repo", "owner", "repo", 1},
	{"ssh://git@github.com/owner/repo.git", "owner", "repo", 1},
	{"ssh://git@github.com/owner/repo", "owner", "repo", 1},
	{"ssh://github.com/owner/repo.git", "owner", "repo", 1},
	{"ssh://user@github.com/owner/repo.git", "owner", "repo", 1},
	{"git+ssh://github.com/owner/repo.git", "owner", "repo", 1},
	{"git+ssh://git@github.com/owner/repo.git", "owner", "repo", 1},
	
	/* Bare URLs */
	{"github.com/owner/repo", "owner", "repo", 1},
	{"github.com/owner/repo.git", "owner", "repo", 1},
	{"github.com:owner/repo.git", "owner", "repo", 1},
	
	/* Edge cases */
	{"https://github.com/owner/repo.git/", "owner", "repo", 1},
	{"https://github.com/owner/repo-with-dashes", "owner", "repo-with-dashes", 1},
	{"https://github.com/owner-with-dashes/repo", "owner-with-dashes", "repo", 1},
	{"https://github.com/OwNeR/RePo", "OwNeR", "RePo", 1},
	{"https://github.com/123/456", "123", "456", 1},
	
	/* Invalid URLs */
	{"https://gitlab.com/owner/repo", NULL, NULL, 0},
	{"file:///path/to/repo.git", NULL, NULL, 0},
	{"https://github.com/", NULL, NULL, 0},
	{"https://github.com/owner", NULL, NULL, 0},
	{"https://github.com/owner/", NULL, NULL, 0},
	{"not-a-url", NULL, NULL, 0},
	{"ftp://github.com/owner/repo", NULL, NULL, 0},
	
	/* Terminator */
	{NULL, NULL, NULL, 0}
};

int main(void)
{
	int total_tests = 0;
	int passed_tests = 0;
	int failed_tests = 0;
	
	printf("GitHub URL Parsing Test Suite\n");
	printf("=============================\n\n");
	
	for (int i = 0; url_tests[i].url != NULL; i++) {
		const struct url_test *test = &url_tests[i];
		char *owner = NULL;
		char *repo = NULL;
		
		total_tests++;
		
		printf("Test %2d: %-50s", total_tests, test->url);
		
		int result = github_parse_repo_url(test->url, &owner, &repo);
		
		if (test->should_succeed) {
			if (result == GITHUB_SUCCESS &&
			    owner && repo &&
			    strcmp(owner, test->expected_owner) == 0 &&
			    strcmp(repo, test->expected_repo) == 0) {
				printf(" [PASS]\n");
				passed_tests++;
			} else {
				printf(" [FAIL]\n");
				printf("         Expected: owner='%s', repo='%s'\n",
				       test->expected_owner, test->expected_repo);
				printf("         Got:      owner='%s', repo='%s', result=%d\n",
				       owner ? owner : "(null)",
				       repo ? repo : "(null)",
				       result);
				failed_tests++;
			}
		} else {
			if (result != GITHUB_SUCCESS) {
				printf(" [PASS] (correctly rejected)\n");
				passed_tests++;
			} else {
				printf(" [FAIL] (should have been rejected)\n");
				printf("         Got: owner='%s', repo='%s'\n",
				       owner ? owner : "(null)",
				       repo ? repo : "(null)");
				failed_tests++;
			}
		}
		
		free(owner);
		free(repo);
	}
	
	printf("\n");
	printf("Test Summary\n");
	printf("============\n");
	printf("Total tests:  %d\n", total_tests);
	printf("Passed:       %d\n", passed_tests);
	printf("Failed:       %d\n", failed_tests);
	printf("Success rate: %.1f%%\n", 
	       total_tests > 0 ? (100.0 * passed_tests / total_tests) : 0.0);
	
	return failed_tests > 0 ? 1 : 0;
}