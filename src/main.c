#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "history.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"

int main(void)
{
    // Display a welcome banner when the shell starts
    printf("=====================================\n");
    printf("      Shellforge \n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");

 token_list_t tokens;
 pipeline_t pipeline;
 
 char *line;

    while (1)
    {
        line = readline("shellforge$ ");
        if (line == NULL)
        {
            printf("\nGoodbye!\n");
            break;
        }
        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

       if (strcmp(line, "history") == 0)
       {
          print_history();
          free(line);
           continue;
       }
// milestone 1 - enabling history

        add_history(line);

// milestone 2.1 - tokenization and lexer

	lexer(line, &tokens);

        token_print(&tokens);

// milestone 2.2 - expansion of environment variables and parser

	if(parser(&tokens, &pipeline))
	{
		expand_variables(&pipeline);
    	        pipeline_print(&pipeline);
	}


	/*
         * ------------------------------------------------
         * BUILTIN TEST
         * ------------------------------------------------
         */

        if (pipeline.command_count > 0)
        {
            command_t *cmd =
                &pipeline.commands[0];

            if (is_builtin(cmd))
            {
                int result =
                    execute_builtin(cmd);

                /*
                 * exit command
                 */
                if (result == 1)
                {
                    free(line);
                    break;
                }
            }
            else
            {
                printf("External command: %s\n",
                       cmd->argv[0]);
            }
        }

       free(line);

    }

    return 0;
}
