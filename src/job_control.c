#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include "job_control.h"

static pid_t shell_pgid;
static int interactive_shell;

void job_control_init(void)
{
    interactive_shell = isatty(STDIN_FILENO);
    shell_pgid = getpgrp();
    if (!interactive_shell) return;

    while (tcgetpgrp(STDIN_FILENO) != (shell_pgid = getpgrp()))
        kill(-shell_pgid, SIGTTIN);

    if (setpgid(0, 0) < 0 && getpgrp() != getpid())
        perror("shellforge: setpgid");
    shell_pgid = getpgrp();
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
        perror("shellforge: tcsetpgrp");
}

void give_terminal_to(pid_t pgid)
{
    if (interactive_shell) (void)tcsetpgrp(STDIN_FILENO, pgid);
}

void take_terminal_back(void)
{
    if (interactive_shell) (void)tcsetpgrp(STDIN_FILENO, shell_pgid);
}

pid_t get_shell_pgid(void) { return shell_pgid; }
