# Code style

The repository treats formatting and control-flow structure as executable
engineering policy. C++ source is formatted with clang-format 17 using the
checked-in `.clang-format` file.

## Control flow

Every `if`, `else`, `for`, `while`, and `do` body uses braces, including a
single statement. A later comment or statement must not silently escape the
condition that appears to govern it.

`switch` statements enumerate every owned enum value. A default arm is used
only for external or byte-level input whose domain is not closed by the source
type system.

## Includes

Includes are grouped by authority:

1. this repository's installed public headers;
2. private headers from the current implementation area;
3. OpenSSL provider headers;
4. C++ standard-library headers.

Public headers include every standard or owner header required by their own
declarations. They do not rely on inclusion order.

## Names and comments

Names describe authority and purpose rather than mechanism history. Comments
explain invariants, protocol ordering, ownership transfer, and reasons that are
not visible from the code. Comments do not paraphrase the next statement.

Public API contracts live in Doxygen comments on installed headers.
Implementation files use ordinary comments for internal invariants and do not
copy the public manual.

## Formatting gate

Run:

```sh
clang-format-17 -i $(find include internal src codec tests \
  -type f \( -name '*.h' -o -name '*.cpp' \) -print)
```

CI runs the same formatter in dry-run mode. A different clang-format major
version is not accepted as a substitute because formatter output is part of the
review surface.
