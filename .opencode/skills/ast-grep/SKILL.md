---
name: ast-grep
description: Use when the user asks to query code structures, search syntax trees, find code patterns, match on AST nodes (kind, metavariables, relational rules), or structurally refactor code across files without parsing raw text. Prefer this over regex/grep for structural code searches. Uses the Docker Desktop ast-grep MCP tool which returns JSON matches.
metadata:
  audience: agents
  workflow: structural-code-search
---

# ast-grep Skill

You are an expert at structural code analysis using `ast-grep`, exposed globally by the Docker Desktop MCP Toolkit.

## When to use me
- User asks to find classes, functions, imports, calls, or other constructs by their structure (not just text).
- User asks to refactor or analyze code patterns across files.
- A regex/grep search would be fragile because the target spans nodes or has unknown content — use AST patterns with metavariables (`$VAR`, `$$$`) instead.

## Tool
The Docker Desktop `mcp/ast-grep` server exposes exactly one MCP tool named `ast-grep`:

- `pattern` (required) — the ast-grep pattern. Must have valid AST structure for the target language.
- `dir` (optional) — directory to search. Defaults to the working directory; the mounted root is the current project.
- `lang` (optional) — language, e.g. `cpp`, `c`, `python`. Auto-detected by file extension when omitted.

Returns a JSON array of matches, each with `text`, `file`, `range`, `lines`, `language`, and captured metavariables.

## Pattern tips
- Metavariables: `$A` (one named node), `$$OP` (one unnamed node), `$$$` (zero+ nodes), `$_` (non-capturing).
- Complex matches: use `kind` (tree-sitter node type, e.g. `call_expression`, `class_specifier`) combined with relational rules (`inside`, `has`) when a simple pattern is ambiguous.
- Non-working metavariables: `obj.on$EVENT`, `"Hello $WORLD"`, `a $OP b`.

## Execution rules
1. **Never use regex text matching** when a structural query is needed; always prefer the ast-grep MCP tool.
2. Do **not** call `find_code`, `find_code_by_rule`, `dump_syntax_tree`, `test_match_code_rule`, `search`, `scan`, or `query` — those are not tools exposed by this server.
3. Trust host paths passed by the environment; the Docker Desktop Toolkit translates workspace directories automatically.
4. If a pattern returns no matches, try adding a `lang`, switching to `kind`, or adding `stopBy: end` to relational rules before giving up.
