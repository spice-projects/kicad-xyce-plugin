# AGENTS.md

## Language

- C++23
- Use RAII
- Avoid raw owning pointers
- Prefer std::span/std::string_view for non-owning views
- Preserve existing architecture unless explicitly requested

## Build & Unit Tests

- Build system: `cmake`
  - Configure: `cmake --preset debug`
  - Build: `cmake --build --preset debug`
- Unit Tests, avoid running unit tests using `ctest`. Execute unit tests by executing the process: `./.build-debug/tests/kicad-xyce-plugin-tests`
- Unit tests must be self-contained: no helper functions, no test utilities, no external fixtures
- Dependencies: `vcpkg`

## Workflow

Before editing:
1. Search for existing implementations
2. Understand ownership and lifetime
3. Follow conventions in `STYLE-GUIDE.md` and the surrounding code
4. Make minimal changes

After editing:
1. Build the project
2. Fix compiler warnings/errors
3. Summarize changed files
4. Do not execute unit tests unless explicitly asked

## Architecture

- wxWidgets owns application lifecycle
- Rendering components should remain independent where possible
- Avoid unnecessary dependency coupling
