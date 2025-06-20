#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "git-mycommand.h"

static void usage(void)
{
	fprintf(stderr, "usage: git mycommand [options] [args]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -h, --help     Show this help message\n");
	fprintf(stderr, "    -v, --verbose  Enable verbose output\n");
	exit(1);
}

static int run_git_command(const char *cmd)
{
	int status;
	pid_t pid = fork();
	
	if (pid == -1) {
		perror("fork");
		return -1;
	}
	
	if (pid == 0) {
		execl("/bin/sh", "sh", "-c", cmd, NULL);
		perror("execl");
		exit(1);
	}
	
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		return -1;
	}
	
	return WEXITSTATUS(status);
}

static int is_git_repository(void)
{
	return run_git_command("git rev-parse --git-dir >/dev/null 2>&1") == 0;
}

int main(int argc, char *argv[])
{
	int verbose = 0;
	int i;
	
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			usage();
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			verbose = 1;
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			usage();
		} else {
			break;
		}
	}
	
	if (!is_git_repository()) {
		fprintf(stderr, "fatal: not a git repository\n");
		return 1;
	}
	
	if (verbose) {
		printf("Running git mycommand in verbose mode\n");
	}
	
	printf("Hello from git mycommand!\n");
	
	if (argc > i) {
		printf("Arguments passed:\n");
		for (; i < argc; i++) {
			printf("  %s\n", argv[i]);
		}
	}
	
	return 0;
}