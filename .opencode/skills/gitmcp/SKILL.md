---
name: gitmcp
description: Use when the user asks about third-party GitHub repositories, libraries, or their documentation — e.g. XyCE, KiCad, Boost, or other upstream C++ dependencies. Fetches live README/docs/llms.txt and code from any public GitHub repo to ground answers and avoid hallucinations. Do NOT use for local repo operations or private code.
metadata:
  audience: agents
  workflow: external-documentation-lookup
---

# GitMCP Skill

You are an expert at using `gitmcp`, exposed globally by the Docker Desktop MCP Toolkit. It turns any public GitHub repository into a documentation and code source so you can answer questions about third-party projects with grounded, up-to-date context.

## When to use me

- The user asks how to use an external library's API, class, or build system (e.g. XyCE, KiCad, Slint, Boost).
- The user asks a question about a specific public GitHub repo, its docs, or its source code.
- The user references a library by name only and you need to resolve it to an owner/repo.
- You are unsure about an upstream API and should verify instead of guessing.

Do NOT use for the current project's own code, local git history, or private/closed repositories — use the built-in tools and `git` commands for those.

## Tools

The Docker Desktop `mcp/gitmcp` server exposes these tools:

- `fetch_generic_documentation` — fetch documentation for any GitHub repo (`owner`, `project`). Prioritizes `llms.txt`, then AI-optimized docs, then `README.md`.
- `search_generic_documentation` — semantic search over a repo's documentation (`owner`, `project`, `query`). Use for specific questions.
- `search_generic_code` — search the repo's code via GitHub code search (`owner`, `project`, `query`). Returns matching files, 30 per page.
- `fetch_generic_url_content` — fetch any absolute URL, respecting robots.txt. Use to retrieve URLs referenced in fetched docs.
- `match_common_libs_owner_repo_mapping` — resolve a bare library name to an `owner`/`repo`. Use only when you do not already know the owner/repo.

## Execution rules

1. If you only have a library name, call `match_common_libs_owner_repo_mapping` first; then use the resolved `owner`/`repo` for the other tools. If it fails, try the name directly.
2. Start with `fetch_generic_documentation` for an overview, then drill down with `search_generic_documentation` or `search_generic_code` for specifics.
3. When a doc page references URLs, follow them with `fetch_generic_url_content` rather than guessing at content.
4. `search_generic_code` returns up to 30 results per page; refine the query instead of paging blindly.
5. Prefer this server over the web for repo-specific API/doc questions — it returns targeted context with far fewer tokens.
