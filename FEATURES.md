# ShellForge 2.0 — Feature Changes

This file lists the additions made on `feature/shellforge-enhancements`.

## 1. New built-in commands

| Command | Purpose |
|---|---|
| `hostname` | Print the system hostname. |
| `sysinfo` | Show OS name, release and machine architecture. |
| `date` | Print the current local date and time. |
| `cinfo [path]` | Show whether a path is a file/directory, its size and permission bits. |
| `dir [path]` | List non-hidden entries in a directory. |
| `which COMMAND` | Find an executable by searching `PATH`. |
| `export NAME=VALUE` | Set an environment variable for the current shell and its child processes. |
| `unset NAME` | Remove an environment variable. |

## 2. Improvements to existing built-ins

- `echo -n` prints without the final newline.
- `cd ~` changes to the user's home directory.
- `history` consistently displays the readline history module.
- `exit [status]` accepts an optional numeric exit status.
- `help` displays the built-ins, aliases and supported operators.
- Built-in command matching is centralized through the built-in dispatch table.

## 3. UI / command-name changes

- Startup banner now displays **ShellForge 2.0**.
- Prompt now displays the current path as `forge:PATH >` and uses `~` for the home directory.
- `commands` is an alias for `help`.
- `quit` is an alias for `exit`.

## 4. Parser and executor improvements

- Parser checks argument count and pipeline length before writing to fixed-size arrays.
- Parser reports missing commands around a pipe and missing filenames after redirection operators.
- Redirection filenames are length-checked before copying.
- Foreground process waiting and the `SIGCHLD` handler are coordinated so the handler does not reap a foreground child before its `waitpid()`.
- Background processes continue to be reaped to reduce zombie processes.
- Environment expansion safely skips null pipeline/argument pointers and continues to support standalone `$NAME` arguments.
- Makefile now has `all`, `run` and `clean` targets, plus stricter compiler warnings.

The project continues to support the existing `fork/execvp` command execution, pipes (`|`), input/output redirection (`<`, `>`, `>>`), background execution (`&`) and readline command history.

## 5. Build and run on Ubuntu

Install the compiler, Make and GNU Readline development files:

```bash
sudo apt update
sudo apt install build-essential libreadline-dev
make clean
make
./shellforge
```

## 6. Manual smoke-test commands

Run these commands inside ShellForge:

```text
help
commands
hostname
sysinfo
date
cinfo Makefile
dir .
which ls
export DEMO=hello
echo $DEMO
unset DEMO
printf 'alpha\\nbeta\\n' | grep beta
echo hello > /tmp/shellforge-test.txt
cat < /tmp/shellforge-test.txt
sleep 2 &
quit
```

## Scope notes

- Variable expansion currently supports a standalone argument such as `$HOME`; embedded expansion such as `prefix$HOME` is not implemented.
- This is an educational Unix-style shell, not a complete Bash replacement. Some advanced shell behavior (for example, command substitution, globbing, job-control signals and full POSIX quoting semantics) is out of scope.
- As in the starting implementation, full parent-shell state changes from built-ins are not meaningful when those built-ins run in a pipeline or background child.
