#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

#include "builtin.h"


/* =========================================================
   BUILTIN: cd
   ========================================================= */

static int builtin_cd(command_t *cmd)
{
    const char *directory;

    if (cmd->argc == 1)
    {
        directory = getenv("HOME");

        if (directory == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
    }
    else if (cmd->argc == 2)
    {
        directory = cmd->argv[1];
    }
    else
    {
        fprintf(stderr, "cd: too many arguments\n");
        return -1;
    }

    if (chdir(directory) != 0)
    {
        perror("cd");
        return -1;
    }

    return 0;
}


/* =========================================================
   BUILTIN: pwd
   ========================================================= */

static int builtin_pwd(command_t *cmd)
{
    char current_directory[4096];

    if (cmd->argc > 1)
    {
        fprintf(stderr, "pwd: too many arguments\n");
        return -1;
    }

    if (getcwd(current_directory,
               sizeof(current_directory)) == NULL)
    {
        perror("pwd");
        return -1;
    }

    printf("%s\n", current_directory);

    return 0;
}


/* =========================================================
   BUILTIN: echo
   ========================================================= */

static int builtin_echo(command_t *cmd)
{
    for (int i = 1; i < cmd->argc; i++)
    {
        printf("%s", cmd->argv[i]);

        if (i < cmd->argc - 1)
        {
            printf(" ");
        }
    }

    printf("\n");

    return 0;
}


/* =========================================================
   BUILTIN: clear
   ========================================================= */

static int builtin_clear(command_t *cmd)
{
    if (cmd->argc > 1)
    {
        fprintf(stderr, "clear: too many arguments\n");
        return -1;
    }

    /*
     * ANSI escape sequence:
     * \033[H -> move cursor to home
     * \033[J -> clear screen
     */
    printf("\033[H\033[J");

    return 0;
}


/* =========================================================
   BUILTIN: help
   ========================================================= */

static int builtin_help(command_t *cmd)
{
    if (cmd->argc > 1)
    {
        fprintf(stderr, "help: too many arguments\n");
        return -1;
    }

    printf("Built-in commands:\n");
    printf("  cd       Change directory\n");
    printf("  pwd      Print current directory\n");
    printf("  echo     Display text\n");
    printf("  clear    Clear terminal\n");
    printf("  help     Show available commands\n");
    printf("  whoami   Show current user\n");
    printf("  history  Show command history\n");
    printf("  exit     Exit shell\n");

    return 0;
}


/* =========================================================
   BUILTIN: whoami
   ========================================================= */

static int builtin_whoami(command_t *cmd)
{
    struct passwd *pw;

    if (cmd->argc > 1)
    {
        fprintf(stderr, "whoami: too many arguments\n");
        return -1;
    }

    pw = getpwuid(getuid());

    if (pw == NULL)
    {
        perror("whoami");
        return -1;
    }

    printf("%s\n", pw->pw_name);

    return 0;
}


/* =========================================================
   BUILTIN: history
   ========================================================= */

static int builtin_history(command_t *cmd)
{
    if (cmd->argc > 1)
    {
        fprintf(stderr, "history: too many arguments\n");
        return -1;
    }

    /*
     * Basic version.
     * Connect this to history.c later.
     */
    printf("history: command history module\n");

    return 0;
}


/* =========================================================
   BUILTIN: exit
   ========================================================= */

static int builtin_exit(command_t *cmd)
{
    if (cmd->argc > 1)
    {
        fprintf(stderr, "exit: too many arguments\n");
        return -1;
    }

    /*
     * Return 1 to tell the main shell loop to terminate.
     */
    return 1;
}


/* =========================================================
   CHECK WHETHER COMMAND IS A BUILTIN
   ========================================================= */

int is_builtin(const command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
    {
        return 0;
    }

    if (strcmp(cmd->argv[0], "cd") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "pwd") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "echo") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "clear") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "help") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "whoami") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "history") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "exit") == 0)
        return 1;

    return 0;
}


/* =========================================================
   EXECUTE BUILTIN
   ========================================================= */

int execute_builtin(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
    {
        return -1;
    }

    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        return builtin_cd(cmd);
    }

    if (strcmp(cmd->argv[0], "pwd") == 0)
    {
        return builtin_pwd(cmd);
    }

    if (strcmp(cmd->argv[0], "echo") == 0)
    {
        return builtin_echo(cmd);
    }

    if (strcmp(cmd->argv[0], "clear") == 0)
    {
        return builtin_clear(cmd);
    }

    if (strcmp(cmd->argv[0], "help") == 0)
    {
        return builtin_help(cmd);
    }

    if (strcmp(cmd->argv[0], "whoami") == 0)
    {
        return builtin_whoami(cmd);
    }

    if (strcmp(cmd->argv[0], "history") == 0)
    {
        return builtin_history(cmd);
    }

    if (strcmp(cmd->argv[0], "exit") == 0)
    {
        return builtin_exit(cmd);
    }

    return -1;
}
