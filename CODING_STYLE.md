# Coding Style Guidelines

This project follows the Git project's coding style guidelines to maintain consistency and readability.

## C Code Formatting

### Indentation and Spacing
- Use **tabs** for indentation (interpreting tabs as 8 spaces)
- Add whitespace around operators and keywords
- No spaces inside parentheses: `if (condition)` not `if ( condition )`
- Space between binary operators: `a + b` not `a+b`
- No space for unary operators: `*ptr` not `* ptr`
- Keep lines to **80 characters maximum**

### Braces and Control Structures
- Avoid unnecessary braces for single statements
- Use braces for multi-line conditions or multiple conditional arms
- Opening brace on same line for functions and control structures

Example:
```c
if (condition)
	single_statement();

if (complex_condition ||
    another_condition) {
	statement1();
	statement2();
}
```

### Variable Declarations and Pointers
- Declare variables at the beginning of blocks before first statement
- Pointers: asterisk sides with variable name: `char *string` not `char* string`
- Use `NULL` for null pointers, not `0`
- Use `static` for file-local variables and functions

### Naming Conventions
- Primary data structures: `struct cache_config`, `struct repo_info`
- Functions operating on structures: `cache_config_create()`, `repo_info_destroy()`
- Use descriptive function names
- File-local functions and variables marked `static`

### Error Handling and Messages
Error messages should:
- Not end with a period
- Not capitalize the first word (unless proper noun)
- State the error first: "failed to create directory" not "directory creation failed"
- Enclose subjects in single quotes: `failed to open 'filename'`

Examples:
```c
/* Good */
fprintf(stderr, "error: failed to open '%s'\n", filename);
fprintf(stderr, "warning: repository already exists\n");

/* Bad */
fprintf(stderr, "Error: Failed to open %s.\n", filename);
fprintf(stderr, "Warning: Repository already exists.\n");
```

### Comments
- Multi-line comments should have delimiters on separate lines:
```c
/*
 * This is a multi-line comment
 * with proper formatting
 */
```

- Single line comments:
```c
/* This is a single line comment */
```

### Control Flow
- Minimize assignments in conditional statements
- Prefer explicit comparisons: `if (ptr != NULL)` over `if (ptr)`
- Avoid overly clever code that reduces readability

### Memory Management
- Use existing APIs when available
- Free allocated memory in reverse order of allocation
- Set pointers to NULL after freeing when appropriate

### Function Structure
- Return early on error conditions
- Use descriptive parameter names
- Group related functionality together

## Shell Script Guidelines (if applicable)
- Use tabs for indentation
- Prefer `$(...)` for command substitution over backticks
- Use POSIX-compliant constructs
- Avoid bashisms for portability

## General Principles
1. **Imitate existing code** in the project
2. **Make code readable and sensible**
3. **Avoid being overly clever**
4. **Prioritize real-world constraints** over strict theoretical standards
5. **Follow the principle of least surprise**

## Code Review Checklist
- [ ] Uses tabs for indentation
- [ ] Lines under 80 characters
- [ ] Variables declared at block start
- [ ] Proper pointer declaration style
- [ ] Error messages follow guidelines
- [ ] No unnecessary braces
- [ ] Descriptive function and variable names
- [ ] Proper memory management
- [ ] Comments are clear and necessary

## Tools
The project can be checked for basic style compliance using:
```bash
# Check for common style issues
grep -n "    " *.c *.h  # Find spaces used instead of tabs
grep -n "/\*.*\*/" *.c *.h  # Find single-line comments on same line
```

For more comprehensive style checking, consider using tools like:
- `checkpatch.pl` (from Linux kernel)
- Custom style scripts based on Git project patterns

## References
- [Git Project Coding Guidelines](https://github.com/git/git/blob/master/Documentation/CodingGuidelines)
- Git source code examples for reference implementations