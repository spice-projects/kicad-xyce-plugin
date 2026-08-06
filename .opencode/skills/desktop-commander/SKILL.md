---
name: desktop-commander
description: Use when running shell/terminal commands, managing long-running processes or sessions, browsing the filesystem, or reading/writing files outside the normal workspace tools. Covers Desktop Commander MCP file operations, process/session management, and file search. Prefer ast-grep for structural code queries and the built-in read/edit/grep tools for ordinary source files.
metadata:
  audience: agents
  workflow: terminal-and-filesystem
---

# Desktop Commander Skill

You are an expert at using `desktop-commander`, exposed globally by the Docker Desktop MCP Toolkit. It provides terminal execution, long-running process management, filesystem operations, and file search.

## When to use me

- The user asks to run a shell command, script, or build step and you need a long-running or interactive terminal.
- The user asks to browse, list, create, move, or read multiple files/directories outside the scope of the built-in tools.
- The user asks to check on, interact with, or kill a running process.
- The user asks to search file contents across a directory.

Do NOT use for ordinary single-file edits or reads inside the project — the built-in `read`/`edit`/`grep`/`glob` tools are cheaper. For structural code queries (find a class, function, call by AST), use the `ast-grep` skill instead.

## Tools

The Docker Desktop `mcp/desktop-commander` server exposes these tools:

### Terminal / processes
- `start_process` — start a terminal command; use for long-running or interactive commands. Pass the command and optionally `wait_for_prompt`.
- `read_process_output` — read accumulated output from a running session.
- `interact_with_process` — send input to a running session (e.g. answers for interactive prompts).
- `list_sessions` — list active terminal sessions (use to check if a session is still alive or blocked).
- `list_processes` — list running processes with PID/CPU/memory.
- `kill_process` / `force_terminate` — terminate a running process by PID.

### Filesystem
- `list_directory` — detailed directory listing (use instead of `ls`). Supports `depth` (default 2); nested dirs cap output at 100 items to avoid context overflow.
- `read_file` — read a single file (or URL).
- `read_multiple_files` — read several files in one call; prefer this to batching single reads.
- `write_file` — write a file.
- `edit_block` — surgical block replacement in a file.
- `create_directory` — create one or more nested directories.
- `move_file` — move/rename.
- `get_file_info` — file metadata.

### Search
- `start_search` — start a file-content search; returns a search ID.
- `get_more_search_results` — fetch further results for an active search.
- `list_searches` — list active searches.
- `stop_search` — stop a running search.

### Meta
- `get_recent_tool_calls` — recent tool call history (recover context for new chats).
- `get_config` / `set_config_value` — view/set server configuration.
- `get_usage_stats`, `get_prompts`, `give_feedback_to_desktop_commander` — rarely needed.

## Execution rules

1. Use **absolute paths** for all file/dir arguments; relative and tilde paths are unreliable. Only paths inside the allowed directories are accessible.
2. For long-running builds (e.g. `cmake --build`), prefer `start_process` over a single-shot call so you can poll `read_process_output` and keep the session alive.
3. If a process seems stuck, check `list_sessions`/`read_process_output` for a `Blocked` prompt before killing it — it may just be waiting for input.
4. Batch reads with `read_multiple_files`; avoid one `read_file` call per file.
5. When a search returns a search ID, call `get_more_search_results` for the rest instead of restarting the search.
6. Do not duplicate work already covered by built-in tools: prefer built-in `read`/`edit`/`glob`/`grep` inside the project, and `ast-grep` for structural queries.
