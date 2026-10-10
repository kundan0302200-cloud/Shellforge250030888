#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "builtin.h"
#include "history.h"
#include "jobs.h"
#include "job_control.h"
#include <signal.h>
#include <sys/wait.h>

/* Built-ins run in the shell process unless used in a pipeline/background. */
static int builtin_cd(command_t *cmd)
{
    const char *directory;
    if (cmd->argc == 1) {
        directory = getenv("HOME");
        if (directory == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
    } else if (cmd->argc == 2) {
        directory = cmd->argv[1];
        if (strcmp(directory, "~") == 0) {
            directory = getenv("HOME");
            if (directory == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return -1;
            }
        }
    } else {
        fprintf(stderr, "cd: usage: cd [directory]\n");
        return -1;
    }
    if (chdir(directory) != 0) {
        perror("cd");
        return -1;
    }
    return 0;
}

static int builtin_pwd(command_t *cmd)
{
    char path[4096];
    if (cmd->argc != 1) {
        fprintf(stderr, "pwd: usage: pwd\n");
        return -1;
    }
    if (getcwd(path, sizeof(path)) == NULL) {
        perror("pwd");
        return -1;
    }
    puts(path);
    return 0;
}

static int builtin_echo(command_t *cmd)
{
    int start = 1;
    int newline = 1;
    if (cmd->argc > 1 && strcmp(cmd->argv[1], "-n") == 0) {
        start = 2;
        newline = 0;
    }
    for (int i = start; i < cmd->argc; i++) {
        if (i > start) putchar(' ');
        fputs(cmd->argv[i], stdout);
    }
    if (newline) putchar('\n');
    return 0;
}

static int builtin_clear(command_t *cmd)
{
    if (cmd->argc != 1) {
        fprintf(stderr, "clear: usage: clear\n");
        return -1;
    }
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
    return 0;
}

static int builtin_help(command_t *cmd)
{
    (void)cmd;
    puts("ShellForge commands:");
    puts("  cd [dir]             Change directory");
    puts("  pwd                  Print working directory");
    puts("  echo [-n] [text...]  Print text");
    puts("  clear                Clear terminal");
    puts("  help / commands      Show this help");
    puts("  whoami               Show current user");
    puts("  history              Show readline history");
    puts("  hostname             Show system hostname");
    puts("  sysinfo              Show basic operating-system info");
    puts("  date                 Show local date and time");
    puts("  cinfo [path]         Show file/directory details");
    puts("  dir [path]           List directory entries");
    puts("  which COMMAND        Locate an executable in PATH");
    puts("  export NAME=VALUE    Set an environment variable");
    puts("  unset NAME           Remove an environment variable");
    puts("  jobs                 List active and stopped jobs");
    puts("  fg JOB_ID            Resume a job in the foreground");
    puts("  bg JOB_ID            Resume a stopped job in the background");
    puts("  exit / quit          Leave ShellForge");
    puts("");
    puts("Shell operators: |, <, >, >>, &, and $VARIABLE expansion.");
    puts("UI aliases: commands=help, dir=ls-like listing, quit=exit.");
    return 0;
}

static int builtin_whoami(command_t *cmd)
{
    struct passwd *pw;
    if (cmd->argc != 1) {
        fprintf(stderr, "whoami: usage: whoami\n");
        return -1;
    }
    pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("whoami");
        return -1;
    }
    puts(pw->pw_name);
    return 0;
}

static int builtin_history(command_t *cmd)
{
    if (cmd->argc != 1) {
        fprintf(stderr, "history: usage: history\n");
        return -1;
    }
    print_history();
    return 0;
}

static int builtin_hostname(command_t *cmd)
{
    struct utsname info;
    if (cmd->argc != 1) {
        fprintf(stderr, "hostname: usage: hostname\n");
        return -1;
    }
    if (uname(&info) != 0) {
        perror("hostname");
        return -1;
    }
    puts(info.nodename);
    return 0;
}

static int builtin_sysinfo(command_t *cmd)
{
    struct utsname info;
    if (cmd->argc != 1) {
        fprintf(stderr, "sysinfo: usage: sysinfo\n");
        return -1;
    }
    if (uname(&info) != 0) {
        perror("sysinfo");
        return -1;
    }
    printf("System : %s\nRelease: %s\nMachine: %s\n",
           info.sysname, info.release, info.machine);
    return 0;
}

static int builtin_date(command_t *cmd)
{
    time_t now;
    struct tm *local;
    char buffer[128];
    if (cmd->argc != 1) {
        fprintf(stderr, "date: usage: date\n");
        return -1;
    }
    now = time(NULL);
    local = localtime(&now);
    if (local == NULL || strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Z %Y", local) == 0) {
        fprintf(stderr, "date: unable to format current time\n");
        return -1;
    }
    puts(buffer);
    return 0;
}

static int builtin_cinfo(command_t *cmd)
{
    const char *path = cmd->argc == 1 ? "." : cmd->argv[1];
    struct stat st;
    if (cmd->argc > 2) {
        fprintf(stderr, "cinfo: usage: cinfo [path]\n");
        return -1;
    }
    if (stat(path, &st) != 0) {
        perror(path);
        return -1;
    }
    printf("Path: %s\nType: %s\nSize: %lld bytes\nPermissions: %o\n",
           path, S_ISDIR(st.st_mode) ? "directory" :
           S_ISREG(st.st_mode) ? "regular file" : "other",
           (long long)st.st_size, (unsigned)(st.st_mode & 0777));
    return 0;
}

static int builtin_dir(command_t *cmd)
{
    const char *path = cmd->argc == 1 ? "." : cmd->argv[1];
    DIR *dir;
    struct dirent *entry;
    if (cmd->argc > 2) {
        fprintf(stderr, "dir: usage: dir [directory]\n");
        return -1;
    }
    dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return -1;
    }
    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') puts(entry->d_name);
    }
    if (errno != 0) perror("dir: readdir");
    closedir(dir);
    return errno == 0 ? 0 : -1;
}

static int builtin_which(command_t *cmd)
{
    const char *path_env;
    char *copy, *saveptr = NULL, *part;
    if (cmd->argc != 2) {
        fprintf(stderr, "which: usage: which COMMAND\n");
        return -1;
    }
    if (strchr(cmd->argv[1], '/')) {
        if (access(cmd->argv[1], X_OK) == 0) puts(cmd->argv[1]);
        else { fprintf(stderr, "which: %s not executable or not found\n", cmd->argv[1]); return 1; }
        return 0;
    }
    path_env = getenv("PATH");
    if (path_env == NULL) {
        fprintf(stderr, "which: PATH not set\n");
        return 1;
    }
    copy = strdup(path_env);
    if (copy == NULL) {
        perror("which");
        return -1;
    }
    for (part = strtok_r(copy, ":", &saveptr); part != NULL;
         part = strtok_r(NULL, ":", &saveptr)) {
        char candidate[4096];
        int n = snprintf(candidate, sizeof(candidate), "%s/%s",
                         *part ? part : ".", cmd->argv[1]);
        if (n > 0 && (size_t)n < sizeof(candidate) && access(candidate, X_OK) == 0) {
            puts(candidate);
            free(copy);
            return 0;
        }
    }
    free(copy);
    fprintf(stderr, "which: %s not found in PATH\n", cmd->argv[1]);
    return 1;
}

static int builtin_export(command_t *cmd)
{
    char *equals;
    if (cmd->argc != 2 || (equals = strchr(cmd->argv[1], '=')) == NULL || equals == cmd->argv[1]) {
        fprintf(stderr, "export: usage: export NAME=VALUE\n");
        return -1;
    }
    *equals = '\0';
    if (setenv(cmd->argv[1], equals + 1, 1) != 0) {
        perror("export");
        *equals = '=';
        return -1;
    }
    *equals = '=';
    return 0;
}

static int builtin_unset(command_t *cmd)
{
    if (cmd->argc != 2) {
        fprintf(stderr, "unset: usage: unset NAME\n");
        return -1;
    }
    if (unsetenv(cmd->argv[1]) != 0) {
        perror("unset");
        return -1;
    }
    return 0;
}

static int builtin_exit(command_t *cmd)
{
    if (cmd->argc > 2) {
        fprintf(stderr, "exit: usage: exit [status]\n");
        return -1;
    }
    if (cmd->argc == 2) {
        char *end = NULL;
        long value = strtol(cmd->argv[1], &end, 10);
        if (end == cmd->argv[1] || *end != '\0') {
            fprintf(stderr, "exit: status must be an integer\n");
            return -1;
        }
        return (int)(value & 0xff);
    }
    return 1; /* Tell main shell loop to exit. */
}

int is_builtin(const command_t *cmd)
{
    static const char *names[] = {
        "cd", "pwd", "echo", "clear", "help", "commands", "whoami",
        "history", "hostname", "sysinfo", "date", "cinfo", "dir",
        "which", "export", "unset", "jobs", "fg", "bg", "exit", "quit", NULL
    };
    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL) return 0;
    for (int i = 0; names[i] != NULL; i++)
        if (strcmp(cmd->argv[0], names[i]) == 0) return 1;
    return 0;
}

static int parse_job_id(command_t *cmd, const char *name)
{
    char *end = NULL;
    long value;
    if (cmd->argc != 2) {
        fprintf(stderr, "%s: usage: %s JOB_ID\\n", name, name);
        return -1;
    }
    value = strtol(cmd->argv[1], &end, 10);
    if (end == cmd->argv[1] || *end != '\\0' || value <= 0 || value > 2147483647L) {
        fprintf(stderr, "%s: invalid job id: %s\\n", name, cmd->argv[1]);
        return -1;
    }
    return (int)value;
}

static int builtin_jobs(command_t *cmd)
{
    if (cmd->argc != 1) {
        fprintf(stderr, "jobs: usage: jobs\\n");
        return -1;
    }
    jobs_print();
    return 0;
}

static int builtin_bg(command_t *cmd)
{
    int id = parse_job_id(cmd, "bg");
    if (id < 0) return -1;
    job_t *job = job_find(id);
    if (job == NULL) {
        fprintf(stderr, "bg: no such job: %d\\n", id);
        return -1;
    }
    if (kill(-job->pgid, SIGCONT) < 0) {
        perror("bg: SIGCONT");
        return -1;
    }
    job_continue(job->pgid);
    printf("[%d] %s &\\n", job->job_id, job->command);
    return 0;
}

static int builtin_fg(command_t *cmd)
{
    int id = parse_job_id(cmd, "fg");
    if (id < 0) return -1;
    job_t *job = job_find(id);
    if (job == NULL) {
        fprintf(stderr, "fg: no such job: %d\\n", id);
        return -1;
    }
    pid_t pgid = job->pgid;
    int status;
    give_terminal_to(pgid);
    if (job->state == JOB_STOPPED && kill(-pgid, SIGCONT) < 0) {
        perror("fg: SIGCONT");
        take_terminal_back();
        return -1;
    }
    job_continue(pgid);
    while (1) {
        pid_t result = waitpid(-pgid, &status, WUNTRACED);
        if (result < 0) {
            if (errno == EINTR) continue;
            if (errno != ECHILD) perror("fg: waitpid");
            break;
        }
        if (WIFSTOPPED(status)) {
            job_stop(pgid);
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            /* For a single-process job, completion is definitive. */
            job_done(pgid);
            break;
        }
    }
    take_terminal_back();
    if (job->state == JOB_DONE) job_remove(id);
    return 0;
}

int execute_builtin(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL) return -1;
    const char *name = cmd->argv[0];
    if (strcmp(name, "commands") == 0) name = "help";
    if (strcmp(name, "quit") == 0) name = "exit";

    if (strcmp(name, "cd") == 0) return builtin_cd(cmd);
    if (strcmp(name, "pwd") == 0) return builtin_pwd(cmd);
    if (strcmp(name, "echo") == 0) return builtin_echo(cmd);
    if (strcmp(name, "clear") == 0) return builtin_clear(cmd);
    if (strcmp(name, "help") == 0) return builtin_help(cmd);
    if (strcmp(name, "whoami") == 0) return builtin_whoami(cmd);
    if (strcmp(name, "history") == 0) return builtin_history(cmd);
    if (strcmp(name, "hostname") == 0) return builtin_hostname(cmd);
    if (strcmp(name, "sysinfo") == 0) return builtin_sysinfo(cmd);
    if (strcmp(name, "date") == 0) return builtin_date(cmd);
    if (strcmp(name, "cinfo") == 0) return builtin_cinfo(cmd);
    if (strcmp(name, "dir") == 0) return builtin_dir(cmd);
    if (strcmp(name, "which") == 0) return builtin_which(cmd);
    if (strcmp(name, "export") == 0) return builtin_export(cmd);
    if (strcmp(name, "unset") == 0) return builtin_unset(cmd);
    if (strcmp(name, "jobs") == 0) return builtin_jobs(cmd);
    if (strcmp(name, "fg") == 0) return builtin_fg(cmd);
    if (strcmp(name, "bg") == 0) return builtin_bg(cmd);
    if (strcmp(name, "exit") == 0) return builtin_exit(cmd);
    return -1;
}
