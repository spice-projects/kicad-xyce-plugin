---
name: style-enforcer
description: Style enforcement for KiCad-Xyce-plugin. Use when generating, modifying, or reviewing Python or QML code to ensure strict compliance with project style standards.
---

# Style Enforcement

This skill ensures all code changes comply with the project's strict style requirements.

## When to use this skill

Always load and consult this skill before:
1. Writing new code.
2. Modifying existing code.
3. Reviewing code for style compliance.

## Reference

- [STYLE_GUIDE.md](../STYLE_GUIDE.md): Complete project style guide.

## Workflow

### 1. Consult the Style Guide

Before starting, ensure you have read and understood the relevant sections in [STYLE_GUIDE.md](../STYLE_GUIDE.md).

### 2. Apply Linting/Formatting

Always prefer automated tools when available:
- For Python: Run `flake8` to catch PEP8 violations.
- Check manually for compliance with project-specific rules (e.g., comment-per-statement rule, blank line restrictions).

### 3. Verify Changes

- Review all changes against the style guide.
- Specifically check:
    - Are comments placed above every statement?
    - Are there any forbidden blank lines?
    - Is the indentation/spacing correct?
    - Are imports ordered correctly?

If you notice style violations, fix them immediately before presenting the code as final.
