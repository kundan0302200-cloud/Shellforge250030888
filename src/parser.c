#include <stdio.h>
#include <string.h>
#include "parser.h"

static void command_init(command_t *cmd)
{
    cmd->argc = 0;
    cmd->input[0] = '\0';
    cmd->output[0] = '\0';
    cmd->append = 0;
    cmd->background = 0;
    for (int i = 0; i < MAX_ARGS; i++) cmd->argv[i] = NULL;
}

int parser(token_list_t *tokens, pipeline_t *pipeline)
{
    int current = 0;
    if (tokens == NULL || pipeline == NULL) return 0;
    pipeline->command_count = 1;
    command_init(&pipeline->commands[0]);

    for (int i = 0; i < tokens->count; i++) {
        token_t *t = &tokens->tokens[i];
        command_t *cmd = &pipeline->commands[current];

        switch (t->type) {
        case TOKEN_WORD:
            if (cmd->argc >= MAX_ARGS - 1) {
                fprintf(stderr, "Parser Error: too many arguments (maximum %d).\n", MAX_ARGS - 1);
                return 0;
            }
            cmd->argv[cmd->argc++] = t->text;
            break;
        case TOKEN_INPUT:
        case TOKEN_OUTPUT:
        case TOKEN_APPEND:
            if (i + 1 >= tokens->count || tokens->tokens[i + 1].type != TOKEN_WORD) {
                fprintf(stderr, "Parser Error: filename expected after %s\n", t->text);
                return 0;
            }
            if (t->type == TOKEN_INPUT) {
                if (strlen(tokens->tokens[i + 1].text) >= MAX_FILENAME) {
                    fprintf(stderr, "Parser Error: input filename too long.\n");
                    return 0;
                }
                strcpy(cmd->input, tokens->tokens[++i].text);
            } else {
                if (strlen(tokens->tokens[i + 1].text) >= MAX_FILENAME) {
                    fprintf(stderr, "Parser Error: output filename too long.\n");
                    return 0;
                }
                strcpy(cmd->output, tokens->tokens[++i].text);
                cmd->append = (t->type == TOKEN_APPEND);
            }
            break;
        case TOKEN_BACKGROUND:
            cmd->background = 1;
            break;
        case TOKEN_PIPE:
            if (cmd->argc == 0) {
                fprintf(stderr, "Parser Error: missing command before '|'.\n");
                return 0;
            }
            if (current + 1 >= MAX_COMMANDS) {
                fprintf(stderr, "Parser Error: too many commands in pipeline (maximum %d).\n", MAX_COMMANDS);
                return 0;
            }
            current++;
            pipeline->command_count++;
            command_init(&pipeline->commands[current]);
            break;
        case TOKEN_END:
            break;
        }
    }

    if (pipeline->commands[current].argc == 0) {
        fprintf(stderr, "Parser Error: missing command at end of input or after '|'.\n");
        return 0;
    }
    pipeline->commands[current].argv[pipeline->commands[current].argc] = NULL;
    return 1;
}

void pipeline_print(const pipeline_t *pipeline)
{
    if (pipeline == NULL) return;
    puts("\n========== SHELLFORGE PIPELINE ==========");
    for (int i = 0; i < pipeline->command_count; i++) {
        const command_t *cmd = &pipeline->commands[i];
        printf("\nCommand %d\n", i + 1);
        for (int j = 0; j < cmd->argc; j++)
            printf("  argv[%d] = %s\n", j, cmd->argv[j]);
        printf("  Input      : %s\n", cmd->input[0] ? cmd->input : "None");
        printf("  Output     : %s\n", cmd->output[0] ? cmd->output : "None");
        printf("  Append     : %s\n", cmd->append ? "Yes" : "No");
        printf("  Background : %s\n", cmd->background ? "Yes" : "No");
    }
    puts("=========================================");
}
