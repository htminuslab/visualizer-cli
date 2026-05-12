# visualizer-cli

A lightweight CLI for Siemens Questa Visualizer. All inputs are on the command line, all output is JSON, making it easy to consume by an LLM. A CLI consumes less token than an mcp server ([Visualizer MCP server](https://github.com/htminuslab/visualizer-mcp)) and is easier to understand and build. The CLI itself is written in good old C. 

## Prerequisites

- GCC and Make
- A running Questa Visualizer server with the VCC (Visualizer Command Center) interface enabled
- `VCC_CLI_INFO` environment variable set (see [Configuration](#configuration))

## Build

Note the windows executable is added to this repository, however, for the obvious security reasons it is strongly advised to build the file from source.

```sh
make
```

Produces `visualizer-cli.exe` on Windows and `visualizer-cli` on Linux. The Makefile detects the platform automatically.

**Linux** requires no extra libraries. **Windows** links against `ws2_32` (Winsock2); build with Questa supplied gcc, MinGW or any GCC toolchain.

## Configuration

Set the `VCC_CLI_INFO` environment variable before running any command.

**Windows**
```bat
:: Connect directly by port (localhost assumed)
set VCC_CLI_INFO=14001

:: Or point to a vccserver.cfg file
set VCC_CLI_INFO=C:\sim\.visualizer\vccserver.cfg
```

**Linux**
```sh
# Connect directly by port (localhost assumed)
export VCC_CLI_INFO=14001

# Or point to a vccserver.cfg file
export VCC_CLI_INFO=/opt/sim/.visualizer/vccserver.cfg
```

A `vccserver.cfg` file contains `port@hostname` on its first line, e.g. `14001@127.0.0.1`.

## Usage

```
visualizer-cli <command> [args...]
```

| Command | Example | Description |
|---------|---------|-------------|
| `help` | `visualizer-cli help` | List all available commands |
| `status` | `visualizer-cli status` | Show server connection info and status |
| `run` | `visualizer-cli run 100ns` | Advance simulation by a time step (or `-all`) |
| `step` | `visualizer-cli step 4` | Single-step N delta cycles |
| `run_status` | `visualizer-cli run_status` | Current simulator run state |
| `get_time` | `visualizer-cli get_time` | Current simulation time |
| `add_wave` | `visualizer-cli add_wave top.clk top.rst` | Add signals to the wave window |
| `force` | `visualizer-cli force top.rst 1 -at 100ns` | Force a signal to a value |
| `examine` | `visualizer-cli examine top.counter` | Read a signal value |
| `eval` | `visualizer-cli eval {wave list}` | Send a raw Tcl command to Visualizer |

## Output format

Every command prints a single JSON object to stdout:

```json
{ "status": "ok", "result": "..." }
{ "status": "error", "message": "..." }
```

`add_wave` returns an array of per-signal results:

```json
{ "status": "ok", "results": [{ "signal": "top.clk", "status": "ok", "result": "..." }] }
```

## Creating a skill

A [Claude Code skill](https://docs.anthropic.com/en/docs/claude-code/skills) teaches Claude when and how to use this CLI. Create the file `.claude/skills/visualizer.md` in your project (or globally at `~/.claude/skills/visualizer.md`):

```markdown
---
name: visualizer
description: Control a Siemens Questa Visualizer simulation from the command line
triggers:
  - user asks to run the simulation
  - user asks to step, advance, or pause the simulation
  - user asks to inspect, read, or force a signal value
  - user asks to add signals to the waveform viewer
  - user wants to check the current simulation time or run status
---

Use `visualizer-cli` to interact with the running Questa Visualizer.

## When to use

Invoke this skill whenever the user wants to:
- Control simulation time (run, step, stop)
- Read or force signal values
- Add signals to the wave window
- Diagnose server connectivity issues

## Setup check

Before issuing any command, verify the server is reachable:

    visualizer-cli status

If the response contains `"status":"error"`, tell the user to check that Questa Visualizer is running
and that `VCC_CLI_INFO` points to the correct port or config file.

## Common tasks

**Run simulation for a duration**

    visualizer-cli run 100ns
    visualizer-cli run -all       # run to completion

**Inspect a signal at the current time**

    visualizer-cli examine <hierarchical.path>

**Force a signal**

    visualizer-cli force <path> <value>
    visualizer-cli force <path> <value> -at <time>

**Add signals to waveform view**

    visualizer-cli add_wave <path1> [path2 ...]

**Raw Tcl access** (escape curly braces as needed by your shell)

    visualizer-cli eval {wave list}

## Output handling

All responses are JSON. Parse `status` first:
- `"ok"` — use the `result` field
- `"error"` — report `message` to the user and suggest a fix

For `add_wave`, iterate the `results` array and report any per-signal errors individually.
```

Claude will automatically load this skill and invoke `visualizer-cli` whenever the simulation context matches one of the triggers above.

## License

See the LICENSE file for details for this demo. 

 
## Notice
All logos, trademarks and graphics used herein are the property of their respective owners.