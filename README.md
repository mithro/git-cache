# git-mycommand

A template for creating custom git subcommands in C.

## Description

This project provides a template for building custom git subcommands in C. Git subcommands are executables that can be invoked as `git <command-name>`. When you name your executable `git-mycommand` and place it in your PATH, you can run it as `git mycommand`.

## Features

- Basic argument parsing with help and verbose options
- Git repository detection
- Proper error handling and exit codes
- Comprehensive test suite
- Easy installation via Makefile

## Building

```bash
make
```

For debugging:
```bash
make debug
```

For static linking:
```bash
make static
```

## Installation

```bash
make install
```

This will install the binary to `/usr/local/bin/`. You can customize the installation prefix:

```bash
make install PREFIX=/opt/local
```

## Usage

```bash
git mycommand [options] [args]
```

### Options

- `-h, --help`: Show help message
- `-v, --verbose`: Enable verbose output

### Examples

```bash
# Basic usage
git mycommand

# With verbose output
git mycommand -v

# With arguments
git mycommand arg1 arg2

# Show help
git mycommand --help
```

## Testing

Run the test suite:

```bash
make test
```

## Customization

To create your own git subcommand:

1. Rename `git-mycommand.c` to `git-yourcommand.c`
2. Update the `TARGET` variable in the Makefile
3. Modify the `PROGRAM_NAME` in `git-mycommand.h`
4. Implement your custom functionality in the main function
5. Update tests and documentation accordingly

## Project Structure

```
.
├── git-mycommand.c     # Main source file
├── git-mycommand.h     # Header file
├── Makefile           # Build configuration
├── README.md          # This file
├── tests/
│   └── run_tests.sh   # Test runner script
└── .gitignore         # Git ignore patterns
```

## Requirements

- GCC or compatible C compiler
- Make
- Git (for testing git repository detection)

## License

This template is provided as-is for educational and development purposes.