
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "parser.h"
#include "executor.h"
#include "builtin.h"
#include "jobs.h"
#include "job_control.h"


/* =========================================================
   BACKGROUND PROCESS HANDLER

   This handler prevents zombie processes.
   ========================================================= */

/* The signal handler must not reap children that foreground waitpid()
   calls still need to collect. */
static volatile sig_atomic_t foreground_waiting = 0;
static volatile sig_atomic_t child_changed = 0;

static void sigchld_handler(int sig)
{
    int saved_errno = errno;
    (void)sig;
    child_changed = 1;
    errno = saved_errno;
}

/* =========================================================
   INSTALL SIGCHLD HANDLER
   ========================================================= */

void setup_background_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = sigchld_handler;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;


    if (sigaction(SIGCHLD, &sa, NULL) < 0)
    {
        perror("sigaction SIGCHLD");
    }
}


/* Reap children deferred while the shell waited for foreground jobs. */
void reap_background_processes(void)
{
    int status;
    pid_t pid;
    int saved_errno = errno;
    if (!child_changed || foreground_waiting) { errno = saved_errno; return; }
    child_changed = 0;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        job_t *job = job_find_by_pgid(pid);
        if (job == NULL) continue;
        if (WIFSTOPPED(status)) {
            job_stop(job->pgid);
            fprintf(stderr, "\n[%d] Stopped %s\n", job->job_id, job->command);
        } else if (WIFCONTINUED(status)) {
            job_continue(job->pgid);
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            job_done(job->pgid);
            fprintf(stderr, "[%d] Done %s\n", job->job_id, job->command);
            job_remove(job->job_id);
        }
    }
    errno = saved_errno;
}

/* =========================================================
   APPLY I/O REDIRECTION
   ========================================================= */

static int apply_redirection(command_t *cmd)
{
    int fd;


    /* =====================================================
       INPUT REDIRECTION: <
       ===================================================== */

    if (cmd->input[0] != '\0')
    {
        fd = open(cmd->input, O_RDONLY);

        if (fd < 0)
        {
            perror(cmd->input);
            return -1;
        }


        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2 input");

            close(fd);

            return -1;
        }

        close(fd);
    }


    /* =====================================================
       OUTPUT REDIRECTION: > and >>
       ===================================================== */

    if (cmd->output[0] != '\0')
    {
        /* ============================
           APPEND: >>
           ============================ */

        if (cmd->append)
        {
            fd = open(
                cmd->output,
                O_WRONLY | O_CREAT | O_APPEND,
                0644
            );
        }

        /* ============================
           OVERWRITE: >
           ============================ */

        else
        {
            fd = open(
                cmd->output,
                O_WRONLY | O_CREAT | O_TRUNC,
                0644
            );
        }


        if (fd < 0)
        {
            perror(cmd->output);

            return -1;
        }


        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 output");

            close(fd);

            return -1;
        }


        close(fd);
    }


    return 0;
}


/* =========================================================
   EXECUTE A SINGLE COMMAND
   ========================================================= */

int execute_command(command_t *cmd)
{
    pid_t pid;

    int status;


    if (cmd == NULL)
    {
        return -1;
    }


    if (cmd->argc == 0)
    {
        return 0;
    }


    /* =====================================================
       BUILTIN COMMAND
       ===================================================== */

    if (is_builtin(cmd))
    {
        /*
         * Builtins such as cd must execute in the shell
         * process for foreground execution.
         */

        if (!cmd->background)
        {
            return execute_builtin(cmd);
        }
    }


    /* =====================================================
       CREATE CHILD / PROCESS GROUP
       ===================================================== */

    foreground_waiting = !cmd->background;
    pid = fork();


    if (pid < 0)
    {
        foreground_waiting = 0;
        reap_background_processes();
        perror("fork");
        return -1;
    }


    /* =====================================================
       CHILD PROCESS
       ===================================================== */

    if (pid == 0)
    {
        char *args[MAX_ARGS + 1];
        (void)setpgid(0, 0);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);


        /* Apply < > >> */

        if (apply_redirection(cmd) < 0)
        {
            _exit(EXIT_FAILURE);
        }


        /* Prepare argv */

        for (int i = 0; i < cmd->argc; i++)
        {
            args[i] = cmd->argv[i];
        }

        args[cmd->argc] = NULL;


        /* =============================================
           BUILTIN IN CHILD

           Needed for background builtin commands.
           ============================================= */

        if (is_builtin(cmd))
        {
            int result = execute_builtin(cmd);

            _exit(result);
        }


        /* =============================================
           EXTERNAL COMMAND
           ============================================= */

        execvp(args[0], args);


        /* execvp returns only on failure */

        perror(args[0]);

        _exit(127);
    }


    /* =====================================================
       PARENT PROCESS
       ===================================================== */


    /* =====================================================
       BACKGROUND PROCESS
       ===================================================== */

    if (cmd->background)
    {
        /* Keep background jobs visible to the jobs builtin. */
        job_add(pid, cmd->argv[0], JOB_RUNNING);
        printf("[Background PID: %d]\n", pid);

        /*
         * DO NOT wait.
         *
         * Shell immediately returns to the prompt.
         */

        return 0;
    }


    /* =====================================================
       FOREGROUND PROCESS
       ===================================================== */

    give_terminal_to(pid);
    if (waitpid(pid, &status, WUNTRACED) < 0)
    {
        foreground_waiting = 0;
        reap_background_processes();
        perror("waitpid");
        return -1;
    }

    take_terminal_back();
    foreground_waiting = 0;
    reap_background_processes();

    if (WIFSTOPPED(status)) {
        int job_id = job_add(pid, cmd->argv[0], JOB_STOPPED);
        fprintf(stderr, "\n[%d] Stopped %s\n", job_id, cmd->argv[0]);
        return 128 + SIGTSTP;
    }

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }

    return -1;
}


/* =========================================================
   EXECUTE PIPELINE
   ========================================================= */

int execute_pipeline(pipeline_t *pipeline)
{
    int command_count;

    int previous_read = -1;

    pid_t pids[MAX_COMMANDS];

    int status;

    int final_status = 0;

    int i;


    /* =====================================================
       VALIDATION
       ===================================================== */

    if (pipeline == NULL)
    {
        return -1;
    }


    command_count = pipeline->command_count;


    if (command_count <= 0)
    {
        return 0;
    }


    /* =====================================================
       CHECK BACKGROUND STATUS

       We use the last command because pipeline_t does not
       have a background member.
       ===================================================== */

    int background =
        pipeline->commands[command_count - 1].background;


    /* =====================================================
       SINGLE COMMAND
       ===================================================== */

    if (command_count == 1)
    {
        return execute_command(
            &pipeline->commands[0]
        );
    }


    /* =====================================================
       MULTI-STAGE PIPELINE
       ===================================================== */

    foreground_waiting = !background;

    for (i = 0; i < command_count; i++)
    {
        int pipefd[2];


        /* =================================================
           CREATE PIPE

           No pipe is needed after the final command.
           ================================================= */

        if (i < command_count - 1)
        {
            if (pipe(pipefd) < 0)
            {
                foreground_waiting = 0;
                reap_background_processes();
                perror("pipe");
                return -1;
            }
        }


        /* =================================================
           CREATE CHILD
           ================================================= */

        pids[i] = fork();


        if (pids[i] < 0)
        {
            foreground_waiting = 0;
            reap_background_processes();
            perror("fork");
            return -1;
        }


        /* =================================================
           CHILD PROCESS
           ================================================= */

        if (pids[i] == 0)
        {
            (void)setpgid(0, i == 0 ? 0 : pids[0]);
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            command_t *cmd =
                &pipeline->commands[i];

            char *args[MAX_ARGS + 1];


            /* =============================================
               INPUT FROM PREVIOUS PIPE
               ============================================= */

            if (previous_read != -1)
            {
                if (dup2(
                        previous_read,
                        STDIN_FILENO
                    ) < 0)
                {
                    perror("dup2 previous pipe");

                    _exit(EXIT_FAILURE);
                }
            }


            /* =============================================
               OUTPUT TO NEXT PIPE
               ============================================= */

            if (i < command_count - 1)
            {
                if (dup2(
                        pipefd[1],
                        STDOUT_FILENO
                    ) < 0)
                {
                    perror("dup2 next pipe");

                    _exit(EXIT_FAILURE);
                }
            }


            /* =============================================
               CLOSE PREVIOUS PIPE
               ============================================= */

            if (previous_read != -1)
            {
                close(previous_read);
            }


            /* =============================================
               CLOSE CURRENT PIPE DESCRIPTORS
               ============================================= */

            if (i < command_count - 1)
            {
                close(pipefd[0]);

                close(pipefd[1]);
            }


            /* =============================================
               APPLY FILE REDIRECTION

               Redirection is applied after pipe setup.

               Example:

               cat file.txt | grep hello > output.txt
               ============================================= */

            if (apply_redirection(cmd) < 0)
            {
                _exit(EXIT_FAILURE);
            }


            /* =============================================
               PREPARE ARGUMENTS
               ============================================= */

            for (int j = 0; j < cmd->argc; j++)
            {
                args[j] = cmd->argv[j];
            }

            args[cmd->argc] = NULL;


            /* =============================================
               BUILTIN COMMAND INSIDE PIPELINE
               ============================================= */

            if (is_builtin(cmd))
            {
                int result =
                    execute_builtin(cmd);

                _exit(result);
            }


            /* =============================================
               EXECUTE EXTERNAL COMMAND
               ============================================= */

            execvp(args[0], args);


            perror(args[0]);


            _exit(127);
        }


        /* =================================================
           PARENT PROCESS
           ================================================= */


        (void)setpgid(pids[i], i == 0 ? pids[i] : pids[0]);

        /* Close previous read descriptor */

        if (previous_read != -1)
        {
            close(previous_read);
        }


        /* Keep read end for next command */

        if (i < command_count - 1)
        {
            close(pipefd[1]);

            previous_read = pipefd[0];
        }

        else
        {
            previous_read = -1;
        }
    }


    /* =====================================================
       BACKGROUND PIPELINE
       ===================================================== */

    if (background)
    {
        /* Register the process-group leader for jobs/fg/bg. */
        job_add(pids[0], pipeline->commands[0].argv[0], JOB_RUNNING);
        printf(
            "[Background Pipeline PID: %d]\n",
            pids[0]
        );


        /*
         * Do not wait for children.
         *
         * SIGCHLD handler will clean them up.
         */

        return 0;
    }


    /* =====================================================
       FOREGROUND PIPELINE

       Wait for every process.
       ===================================================== */

    give_terminal_to(pids[0]);
    for (i = 0; i < command_count; i++)
    {
        status = 0;


        if (waitpid(pids[i], &status, 0) < 0)
        {
            perror("waitpid");

            continue;
        }


        /*
         * Pipeline exit status is normally taken from
         * the last command.
         */

        if (i == command_count - 1)
        {
            if (WIFEXITED(status))
            {
                final_status =
                    WEXITSTATUS(status);
            }

            else
            {
                final_status = -1;
            }
        }
    }


    take_terminal_back();
    foreground_waiting = 0;
    reap_background_processes();
    return final_status;
}
