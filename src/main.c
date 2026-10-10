#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "history.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"
#include "executor.h"

static void show_banner(void)
{
    puts("============================================");
    puts("             ShellForge 2.0");
    puts("     A lightweight Unix-style shell in C");
    puts("============================================");
    puts("Type 'help' or 'commands' to see available commands.");
}

int main(void)
{
    token_list_t tokens;
    pipeline_t pipeline;
    char *line;

    show_banner();
    setup_background_handler();
    using_history();

    while (1) {
        char prompt[512];
        char cwd[400];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            const char *home = getenv("HOME");
            if (home != NULL && strncmp(cwd, home, strlen(home)) == 0 &&
                (cwd[strlen(home)] == '/' || cwd[strlen(home)] == '\0')) {
                snprintf(prompt, sizeof(prompt), "forge:%s%s > ",
                         "~", cwd + strlen(home));
            } else {
                snprintf(prompt, sizeof(prompt), "forge:%s > ", cwd);
            }
        } else {
            snprintf(prompt, sizeof(prompt), "forge > ");
        }

        line = readline(prompt);
        if (line == NULL) {
            puts("\nShellForge closed.");
            break;
        }
        if (line[0] == '\0') {
            free(line);
            continue;
        }

        add_history(line);
        lexer(line, &tokens);
        if (!parser(&tokens, &pipeline)) {
            free(line);
            continue;
        }

        if (pipeline.command_count == 1 && pipeline.commands[0].argc > 0 &&
            (strcmp(pipeline.commands[0].argv[0], "exit") == 0 ||
             strcmp(pipeline.commands[0].argv[0], "quit") == 0)) {
            int status = execute_builtin(&pipeline.commands[0]);
            free(line);
            if (status == 1) break;
            if (status >= 0) {
                /* A numeric exit status also exits; zero is handled above. */
                break;
            }
            continue;
        }

        expand_variables(&pipeline);
        (void)execute_pipeline(&pipeline);
        free(line);
    }
    return 0;
}
