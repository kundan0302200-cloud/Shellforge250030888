# ShellForge

ShellForge is a lightweight Unix-style command-line shell written in C. It uses GNU Readline for interactive input/history and implements a lexer, parser, built-in commands and process executor.

## Build

Ubuntu / Debian:

```bash
sudo apt update
sudo apt install build-essential libreadline-dev
make clean
make
./shellforge
```

Or run it with `make run` after the dependencies are installed.

## Features

- Built-ins: `cd`, `pwd`, `echo`, `clear`, `help`, `whoami`, `history`, `hostname`, `sysinfo`, `date`, `cinfo`, `dir`, `which`, `export`, `unset`, `exit`.
- UI aliases: `commands` for help, and `quit` for exit.
- External command execution through `fork()` and `execvp()`.
- Pipelines with `|`.
- Input/output redirection with `<`, `>` and `>>`.
- Background commands with `&`.
- Standalone environment-variable expansion such as `echo $HOME`.
- Readline command history.
- Current-directory prompt: `forge:PATH >`.

See [FEATURES.md](FEATURES.md) for the change list and manual smoke-test checklist.

## Project layout

- `src/`: implementation for built-ins, lexer, parser, expansion, history and executor.
- `include/`: public interfaces and shared structures.
- `tests/`: location for future automated tests.
- `docs/`: project documentation.

ShellForge is a learning project and does not aim to implement every feature or edge case of Bash.
