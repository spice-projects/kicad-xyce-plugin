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
- Private methods prefix (_): TBD

### Class Formatting
- Class names (PascalCase): TBD
- Spacing around class definitions: TBD

### Comments
- Inline comments style: Comments placed above the code they describe, not inline. Format: `# comment text` (starts with lowercase letter, no period)
- Every non-trivial statement gets its own comment line above it — including statements inside `if` blocks, loops, and other control structures
- Docstring format (Google, NumPy, etc.): TBD

### Line Breaks
- Blank lines between functions: Two blank lines
- Blank lines between classes: TBD
- Within function bodies: No blank lines — comments above each statement serve as the only visual separators

### Type Hints
- Use type hints: TBD
- Format preference: TBD

### Other Preferences
- Two blank lines between import block and first function definition
- Two blank lines between function definitions and `if __name__ == '__main__'` block
- Function calls are always written on a single line — no multiline call syntax (no trailing `(`, no argument continuation lines)


## QML Formatting

### Declarations
- Signal and property declarations are always written on a single line — no multiline continuation


## Testing

### Framework
- Use the standard library `unittest` module (`unittest.TestCase`)
- No third-party test runners or assertion libraries (no pytest, no assertpy, etc.)

### File & Class Naming
- Test files live under the `tests/` directory and are named `<module>_test.py`
- One `TestCase` subclass per file, named `Test<ClassUnderTest>` (PascalCase)

### Method Naming
- Test method names use snake_case: `test_<what_is_being_tested>`
- Names should be descriptive enough to understand the scenario without reading the body

### Structure — Arrange / Act / Assert
- Every test body is divided into three sections marked with lowercase comments:
  ```python
  # arrange
  ...
  # act
  ...
  # assert
  ...
  ```
- No blank lines between or within sections — the comments themselves serve as visual separators
- All test methods must be fully self-contained. Do not use setUp, tearDown, or class-level fixtures. All test data and setup must be defined inside the test method itself.

### Assertions
- Use `self.assert*` methods from `TestCase` — one assertion per line
- Prefer specific assertion methods (`assertEqual`, `assertTrue`, `assertIsNotNone`, etc.) over generic `assert`
- Group related assertions together without blank lines between them

### Fixtures & Test Data
- Fixture files live under `tests/PyQSPICE/` (or a sibling directory next to the test file)
- Define a module-level constant for the fixtures directory using `Path(__file__).parent`:
  ```python
  FIXTURES_DIR = Path(__file__).parent / "PyQSPICE"
  ```
- Construct fixture paths inside the test using the constant: `FIXTURES_DIR / "file.qraw"`
- All test data must be defined inside the test method. Do not use setUp or tearDown for test data or fixtures.

### Comments
- Follow the same comment style as production code: above the line, lowercase, no period
- The `# arrange`, `# act`, `# assert` markers are the only required comments
- Add extra comments only when the intent of a step is not obvious from the code
