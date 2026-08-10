# Manual-page Markdown

## Purpose

Manual pages are authored in a restricted Markdown profile and converted to
`man(7)` roff with Pandoc. Markdown is canonical. Generated roff is committed as
derived release material so ordinary builds do not require Pandoc.

## Canonical and generated files

Canonical sources live in `docs/man/*.md`. Generated pages live in
`docs/man/generated/` and retain their installed section suffix.

Do not edit generated roff directly. Use:

```sh
tools/update-man-pages.sh --write
tools/update-man-pages.sh --check
```

Configured Meson trees expose equivalent targets when Pandoc is available.

## Document header

Each source starts with one Pandoc title block:

```text
% PAGE_NAME(SECTION) libpkgsource 3.0.1 | libpkgsource
```

The `NAME` section carries the lowercase installed name and one concise purpose
statement separated by an ASCII hyphen.

## Allowed Markdown

The profile permits paragraphs, lists, definition lists, fenced code blocks,
inline code, and semantic emphasis. Use fenced blocks for exact C++ declarations
and shell commands.

## Forbidden Markdown

Manual sources must not contain raw roff, raw HTML, Setext headings, horizontal
rules, tables, footnotes, citations, images, task lists, automatic or reference
links, tabs, or trailing whitespace.

## Conversion contract

The generator supports Pandoc 3.1 through 3.x. It disables syntax highlighting,
uses LF output without wrapping, treats warnings as failures, and passes the man
writer output through the project-owned canonicalizer.

Canonicalization removes writer-added font escapes inside exact examples and
normalizes equivalent two-character roff special-character spellings. A new
Pandoc major version requires explicit compatibility review.

Generated roff is compared byte-for-byte after canonicalization and must pass
`mandoc -Tlint`.
