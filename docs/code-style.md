# Code style

The repository treats formatting and control-flow structure as executable
engineering policy. C++ source is formatted with `clang-format 17` using the
checked-in `.clang-format` file.

## Control flow

Control statements always use braces, including a single statement. A later
comment or statement must not silently escape the condition that appears to
govern it.

`switch` statements enumerate every owner-controlled enum value. A default arm
is reserved for external byte input whose domain is not closed by the source
type system.

## Includes

Includes are grouped by authority:

1. installed public headers from this repository;
2. private headers from the current implementation area;
3. provider headers;
4. C++ standard-library headers.

Public headers include every owner or standard header required by their own
declarations. They do not rely on inclusion order.

## Names and comments

Names describe authority and purpose rather than mechanism history. Comments
explain invariants, protocol ordering, ownership transfer, and reasons that are
not visible from the code. Comments do not paraphrase the next statement.

Public API contracts live in Doxygen comments on installed headers.
Implementation files use ordinary comments for internal invariants and do not
copy the public manual.

## Documentation

Markdown uses ATX headings only. Repository Markdown does not carry SPDX HTML
comments; licensing authority remains in `COPYING` and `COPYRIGHT`.

Manual pages use the restricted profile in `docs/manpage-markdown.md`.
Generated roff is derived output and is never edited directly.

## Formatting gate

Run:

```sh
clang-format-17 -i $(find include internal src codec tests \
  -type f \( -name '*.h' -o -name '*.cpp' \) -print)
```

CI runs the same formatter in dry-run mode. Another clang-format major is not a
substitute because formatter output is part of the review surface.
