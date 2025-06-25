/**
 * @file test_submodule.c
 * @brief Tests for git submodule support
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

#include "git-cache.h"
#include "submodule.h"

static void test_gitmodules_parsing(void)
{
	printf("=== Testing .gitmodules Parsing ===\n");
	
	/* Create a test .gitmodules file */
	const char *test_file = "/tmp/test_gitmodules";
	FILE *f = fopen(test_file, "w");
	assert(f != NULL);
	
	fprintf(f, "[submodule \"lib/helper\"]\n");
	fprintf(f, "\tpath = lib/helper\n");
	fprintf(f, "\turl = https://github.com/user/helper.git\n");
	fprintf(f, "\n");
	fprintf(f, "[submodule \"vendor/tool\"]\n");
	fprintf(f, "\tpath = vendor/tool\n");
	fprintf(f, "\turl = git@github.com:org/tool.git\n");
	fprintf(f, "\tbranch = stable\n");
	fclose(f);
	
	/* Parse the file */
	struct submodule_list list;
	int ret = parse_gitmodules(test_file, &list);
	printf("Parse returned: %d, found %zu submodules\n", ret, list.count);
	assert(ret == 0);
	assert(list.count == 2);
	
	/* Verify first submodule */
	assert(strcmp(list.submodules[0].name, "lib/helper") == 0);
	assert(strcmp(list.submodules[0].path, "lib/helper") == 0);
	assert(strcmp(list.submodules[0].url, "https://github.com/user/helper.git") == 0);
	assert(list.submodules[0].branch[0] == '\0');
	printf("✓ First submodule parsed correctly\n");
	
	/* Verify second submodule */
	assert(strcmp(list.submodules[1].name, "vendor/tool") == 0);
	assert(strcmp(list.submodules[1].path, "vendor/tool") == 0);
	assert(strcmp(list.submodules[1].url, "git@github.com:org/tool.git") == 0);
	assert(strcmp(list.submodules[1].branch, "stable") == 0);
	printf("✓ Second submodule parsed correctly\n");
	
	free_submodule_list(&list);
	unlink(test_file);
	
	printf("✓ All parsing tests passed\n");
}

static void test_empty_gitmodules(void)
{
	printf("\n=== Testing Empty .gitmodules ===\n");
	
	/* Create empty file */
	const char *test_file = "/tmp/test_empty_gitmodules";
	FILE *f = fopen(test_file, "w");
	assert(f != NULL);
	fclose(f);
	
	struct submodule_list list;
	int ret = parse_gitmodules(test_file, &list);
	assert(ret == 0);
	assert(list.count == 0);
	/* For empty file, we still allocate the array */
	
	printf("✓ Empty file handled correctly\n");
	
	unlink(test_file);
}

static void test_missing_gitmodules(void)
{
	printf("\n=== Testing Missing .gitmodules ===\n");
	
	struct submodule_list list;
	int ret = parse_gitmodules("/tmp/nonexistent_gitmodules", &list);
	assert(ret == 0);
	assert(list.count == 0);
	assert(list.submodules == NULL);
	
	printf("✓ Missing file handled correctly\n");
}

static void test_malformed_gitmodules(void)
{
	printf("\n=== Testing Malformed .gitmodules ===\n");
	
	const char *test_file = "/tmp/test_malformed_gitmodules";
	FILE *f = fopen(test_file, "w");
	assert(f != NULL);
	
	/* Submodule with missing URL */
	fprintf(f, "[submodule \"incomplete\"]\n");
	fprintf(f, "\tpath = some/path\n");
	fprintf(f, "\n");
	
	/* Valid submodule */
	fprintf(f, "[submodule \"complete\"]\n");
	fprintf(f, "\tpath = valid/path\n");
	fprintf(f, "\turl = https://example.com/repo.git\n");
	fclose(f);
	
	struct submodule_list list;
	int ret = parse_gitmodules(test_file, &list);
	assert(ret == 0);
	assert(list.count == 1);
	assert(strcmp(list.submodules[0].name, "complete") == 0);
	
	printf("✓ Malformed entries skipped correctly\n");
	
	free_submodule_list(&list);
	unlink(test_file);
}

int main(void)
{
	printf("Submodule Support Test Suite\n");
	printf("============================\n\n");
	
	test_gitmodules_parsing();
	test_empty_gitmodules();
	test_missing_gitmodules();
	test_malformed_gitmodules();
	
	printf("\n=== Test Summary ===\n");
	printf("All submodule tests passed!\n");
	
	return 0;
}