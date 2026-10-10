#ifndef PARSER_H
#define PARSER_H

#include "token.h"

#define MAX_COMMANDS   16
#define MAX_ARGS       32
#define MAX_FILENAME   128

typedef struct
{
    char *argv[MAX_ARGS];
    int argc;
    char input[MAX_FILENAME];
    char output[MAX_FILENAME];
    int append;
    int background;
} command_t;

typedef struct
{
    command_t commands[MAX_COMMANDS];
    int command_count;
} pipeline_t;

int parser(token_list_t *tokens, pipeline_t *pipeline);
void pipeline_print(const pipeline_t *pipeline);

#endif
