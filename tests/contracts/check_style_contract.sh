#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=$1
build_root=$2

fail()
{
  echo "style-contract-test: $*" >&2
  exit 1
}

for file in .clang-format .editorconfig docs/code-style.md docs/manpage-markdown.md Doxyfile; do
  [ -s "$root/$file" ] || fail "$file is missing or empty"
done

markdown=$(find "$root" -path "$root/.git" -prune -o -path "$build_root" -prune -o -type f -name '*.md' -print)

if grep -n -E 'SPDX-(FileCopyrightText|License-Identifier)' $markdown >/dev/null; then
  fail 'Markdown contains SPDX comments; use COPYING and COPYRIGHT'
fi

if grep -n -E '^(===+|---+)[[:space:]]*$' $markdown >/dev/null; then
  fail 'Markdown contains Setext headings or horizontal rules'
fi

if grep -n "$(printf '\t')" $markdown >/dev/null; then
  fail 'Markdown contains tab indentation'
fi

if grep -n -E '[[:blank:]]+$' $markdown >/dev/null; then
  fail 'Markdown contains trailing whitespace'
fi

grep -F '# Code style' "$root/docs/code-style.md" >/dev/null ||
  fail 'docs/code-style.md does not declare its purpose'
grep -F 'Control statements always use braces' "$root/docs/code-style.md" >/dev/null ||
  fail 'docs/code-style.md does not bind braced control flow'
grep -F '`clang-format 17`' "$root/docs/code-style.md" >/dev/null ||
  fail 'docs/code-style.md does not pin the formatter major'
grep -F 'Markdown uses ATX headings only' "$root/docs/code-style.md" >/dev/null ||
  fail 'docs/code-style.md does not bind Markdown headings'
grep -F 'Manual pages use the restricted profile in `docs/manpage-markdown.md`' \
  "$root/docs/code-style.md" >/dev/null ||
  fail 'docs/code-style.md does not bind the manual-page profile'
