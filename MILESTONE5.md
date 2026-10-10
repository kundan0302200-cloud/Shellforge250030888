# Milestone 5 — Job Control

This milestone adds basic POSIX job-control support while preserving the existing lexer, parser, variable expansion, built-ins, and shell UI.

## Implemented

- Job table with job IDs, process-group IDs, command descriptions and Running/Stopped/Done states.
- `jobs`: list registered jobs.
- `fg JOB_ID`: transfer terminal control to a job, continue it if stopped, wait for it, then return terminal control to ShellForge.
- `bg JOB_ID`: send SIGCONT to a stopped job without taking terminal control.
- Process groups for single background commands and pipeline children.
- Terminal foreground process-group switching for foreground commands.
- SIGCHLD handler only records a child-state change; normal shell code reaps children to avoid doing job-table operations inside a signal handler.
- Initializes job table and terminal job control when ShellForge starts.

## Files added

- `include/jobs.h`, `src/jobs.c`: job table and state management.
- `include/job_control.h`, `src/job_control.c`: terminal/process-group helpers.

## Minimal source changes

- `src/builtin.c`: registers and dispatches `jobs`, `fg`, `bg`.
- `src/executor.c`: process groups, foreground terminal handoff, background job registration and child-state reaping.
- `src/main.c`: initializes job table and job-control support.
- `include/executor.h`: exports the existing child-reaper function (already used by the current shell).

## Build and run (Ubuntu)

```bash
sudo apt update
sudo apt install build-essential libreadline-dev
make clean
make
./shellforge
```

## Manual test

```text
sleep 60 &
jobs
fg 1
```

While `sleep` is running in the foreground, press Ctrl+Z to stop it. Back at the shell prompt, run `jobs`, then `bg 1`, `jobs`, and `fg 1`. Press Ctrl+C while it is foregrounded to interrupt it.

To test a background pipeline, try `sleep 20 | cat &`, then `jobs`.

## Scope / limitations

This is a compact educational implementation. Terminal job control requires an interactive terminal (TTY); non-interactive use skips terminal handoff. Pipeline job completion is simplified compared with a production shell, and more advanced job-control behavior can be added in a later milestone.
