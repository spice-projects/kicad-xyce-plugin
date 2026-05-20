# Code Style Guide

This document defines the code style preferences for this project.

## Python Formatting

### Indentation

- Spaces or tabs: Spaces
- Width: 4 spaces

### Line Length

- Maximum line length: Standard (appears to follow PEP 8)

### Quotes

- Single or double quotes: Double quotes
- When to use docstrings: TBD

### Imports

- Import order: three sections separated by a blank line:
  1. Standard library
  2. Third-party libraries
  3. Project files (local imports)
- Within each section: `import ...` statements first (alphabetical), then `from ... import ...` statements (alphabetical)
- One import per line

### Function/Variable Naming

- Snake case, camelCase, PascalCase: snake_case for functions and variables
- Private methods prefix (_): '_'

### Class Formatting

- Class names (PascalCase): PascalCase
- Spacing around class definitions: TBD

### Comments

- Inline comments style: Comments placed above the code they describe, not inline. Format: `# comment text` (starts with lowercase letter, no period)
- Every non-trivial statement gets its own comment line above it — including statements inside `if` blocks, loops, and other control structures
- Docstring format (Google, NumPy, etc.): TBD

### Line Breaks

- Blank lines between functions: Two blank lines
- Blank lines between classes: Two blank lines
- Within function bodies: No blank lines — comments above each statement serve as the only visual separators

### Type Hints

- Use type hints: TBD
- Format preference: TBD

### Other Preferences

- Two blank lines between import block and first function definition
- Two blank lines between function definitions and `if __name__ == '__main__'` block
- Function calls are always written on a single line — no multiline call syntax (no trailing `(`, no argument continuation lines)
- Function definitions are always written on a single line — no multiline definition syntax (no trailing `(`, no argument continuation lines)

## QML Formatting

### Declarations

- Signal and property declarations are always written on a single line — no multiline continuation

## Testing

### Framework

- Use the library `pytest`

### File & Class Naming

- Test files live under the `tests/` directory and are named `<module>_test.py`
- Use plain `Test...` classes for grouping related tests

### Method Naming

- Test method names use snake_case: `test_<what_is_being_tested>`
- Names should be descriptive enough to understand the scenario without reading the body

### Structure — Arrange / Act / Assert

- Test bodies are commonly divided into sections marked with lowercase comments:
  ```python
  # arrange
  ...
  # act
  ...
  # assert
  ...
  ```
- `# act/assert` is also acceptable when a single step both executes and verifies behavior
- Separate each section with a single blank line for readability
- All test methods should be self-contained whenever possible. Avoid `setUp`, `tearDown`, or class-level test fixtures unless there is a strong reason to share setup across multiple tests

### Assertions

- Use bare `assert` statements, not `unittest.TestCase` assertion methods
- Prefer explicit expressions and specific helpers such as `pytest.approx` for fuzzy numeric comparisons
- Use one assertion per line and group related assertions together without blank lines between them

### Fixtures & Test Data

- Fixture files are kept near the tests, for example under `tests/<subdir>/test-suite`
- Define fixture directory paths with a module-level constant when needed:
  ```python
  FIXTURES_DIR = Path(__file__).parent / "test-suite"
  ```
- Construct fixture paths inside the test using the constant: `FIXTURES_DIR / "file.txt"`
- Keep test data setup local to the test method unless sharing makes the tests clearer and avoids duplication

### Comments

- Follow the same comment style as production code: above the line, lowercase, no period
- The `# arrange`, `# act`, `# assert` markers are the primary section headings
- Add extra comments only when the intent of a step is not obvious from the code
