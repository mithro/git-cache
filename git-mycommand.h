#ifndef GIT_MYCOMMAND_H
#define GIT_MYCOMMAND_H

#define VERSION "1.0.0"
#define PROGRAM_NAME "git-mycommand"

/* Function prototypes */
static void usage(void);
static int run_git_command(const char *cmd);
static int is_git_repository(void);

#endif /* GIT_MYCOMMAND_H */