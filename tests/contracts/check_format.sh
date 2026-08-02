#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

formatter=$1
root=$2

fail()
{
  echo "format-test: $*" >&2
  exit 1
}

version=$($formatter --version 2>/dev/null) || fail "cannot execute $formatter"
case $version in
  *'clang-format version 17.'*) ;;
  *) fail "expected clang-format 17, found: $version" ;;
esac

find "$root/include" "$root/src" "$root/codec" "$root/internal" "$root/tests" \
  -type f \( -name '*.h' -o -name '*.cpp' \) -print0 |
  sort -z |
  xargs -0 "$formatter" --dry-run --Werror
