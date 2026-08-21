# Code Style Guide

This document defines the code style preferences for this project.

## C++ Formatting

### Indentation

- 4 spaces per indent
- No tabs

### Line Length

- No hard limit; prefer readability over strict wrapping

### Braces

- Class definitions: opening brace on its own line
- Function definitions: opening brace on the same line as the signature
- Control structures (`if`, `for`, `while`): opening brace on the same line
- Braces required unless the body is a single line (no braces for single-line bodies); if a comment makes the body span two lines, braces are required

### Naming

- Class names: `PascalCase`
- Functions and variables: `snake_case`
- Member variables: `m_` prefix with snake_case
- Template parameters: `T`, `U`, `Key`, `Value`
- Constants: `UPPER_SNAKE_CASE` for file-scope constants and enum values

### Includes

- Three sections separated by a blank line:
  1. Standard library (alphabetical)
  2. Third-party libraries (alphabetical)
  3. Project files (relative paths, alphabetical)
- No `#include` paths with `..` when the file lives in a nearby directory

### Header Guards

- `#pragma once` only

### Comments

- Comments are placed **above** the code they describe, not inline
- Format: `// comment text` (starts with lowercase letter, no period), single line
- Every non-trivial statement gets its own comment line above it — including statements inside `if` blocks, loops, and other control structures
- No docstring format; use plain `//` comments

### Classes

- Public section first, then private
- Use `= default` and `= delete` for special member functions
- Prefer `explicit` for single-argument constructors
- Prefer `[[nodiscard]]` for accessors and functions where ignoring the return value is likely an error
- Use member initializer lists in constructors

### Line Breaks

- One blank line between function definitions
- A maximum of one blank line between sections inside a function
- One blank line between include sections

### Misc

- Use `auto` where it aids readability (iterators, casts, long type names)
- Prefer `std::span` and `std::string_view` for non-owning views
- Use `nullptr`, not `NULL` or `0`
- Use `override` on all overridden virtual functions
- Return `const&` from accessors
- Pass by value and `std::move` for sink parameters

## Testing

### Framework

- Use Google Test (`gtest`)

### File Naming

- Test files live under the `tests/` directory and mirror the source tree layout
- Named `<module>.test.cpp`

### Naming

- Test suite name: `PascalCase` (e.g. `DCSimulationParametersChecks`)
- Test case name: descriptive `snake_case` string (e.g. `parses_lin_sweep`)

### Structure — Arrange / Act / Assert

- Every test **must** use the `arrange, act, assert` format with explicit section-comment markers
  ```cpp
  // arrange
  ...
  // act
  ...
  // assert
  ...
  ```
- `// arrange / act` is also acceptable when setup and execution are a single step
- Do not separate each section with blank lines
- All test methods should be self-contained whenever possible, avoid utility functions

### Assertions

- Use `ASSERT_*` and `EXPECT_*` macros from Google Test, not `assert`
- Prefer `ASSERT_EQ`, `ASSERT_TRUE`, `ASSERT_FALSE`, `ASSERT_THROW`
- Use one assertion per line and group related assertions together without blank lines between them
