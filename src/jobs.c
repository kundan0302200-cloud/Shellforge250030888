#include <stdio.h>
#include <string.h>
#include "jobs.h"

static job_t job_table[MAX_JOBS];
static int next_job_id = 1;

void jobs_init(void)
{
    memset(job_table, 0, sizeof(job_table));
    next_job_id = 1;
}

int job_add(pid_t pgid, const char *command, job_state_t state)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].job_id == 0) {
            job_table[i].job_id = next_job_id++;
            job_table[i].pgid = pgid;
            job_table[i].state = state;
            snprintf(job_table[i].command, sizeof(job_table[i].command), "%s",
                     command ? command : "(command)");
            return job_table[i].job_id;
        }
    }
    fprintf(stderr, "jobs: maximum number of jobs reached\n");
    return -1;
}

job_t *job_find(int job_id)
{
    for (int i = 0; i < MAX_JOBS; i++)
        if (job_table[i].job_id == job_id) return &job_table[i];
    return NULL;
}

job_t *job_find_by_pgid(pid_t pgid)
{
    for (int i = 0; i < MAX_JOBS; i++)
        if (job_table[i].job_id != 0 && job_table[i].pgid == pgid) return &job_table[i];
    return NULL;
}

void job_remove(int job_id)
{
    job_t *job = job_find(job_id);
    if (job) memset(job, 0, sizeof(*job));
}

void job_stop(pid_t pgid)
{
    job_t *job = job_find_by_pgid(pgid);
    if (job) job->state = JOB_STOPPED;
}

void job_continue(pid_t pgid)
{
    job_t *job = job_find_by_pgid(pgid);
    if (job) job->state = JOB_RUNNING;
}

void job_done(pid_t pgid)
{
    job_t *job = job_find_by_pgid(pgid);
    if (job) job->state = JOB_DONE;
}

void jobs_print(void)
{
    const char *states[] = {"Running", "Stopped", "Done"};
    int any = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].job_id) {
            printf("[%d] %-8s %s\n", job_table[i].job_id,
                   states[job_table[i].state], job_table[i].command);
            any = 1;
        }
    }
    if (!any) puts("No jobs.");
}
