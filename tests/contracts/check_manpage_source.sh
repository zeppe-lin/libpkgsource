#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail()
{
  echo "manpage-source: $*" >&2
  exit 1
}

check_page()
{
  page=$1
  title=$2
  name_line=$3
  source=$root/docs/man/$page.md
  [ -s "$source" ] || fail "missing canonical source: $page.md"
  [ "$(sed -n '1p' "$source")" = "$title" ] ||
    fail "title block is wrong: $page.md"
  actual_name=$(awk '
    /^# NAME$/ { in_name = 1; next }
    /^# / && in_name { exit }
    in_name && NF { print; exit }
  ' "$source")
  [ "$actual_name" = "$name_line" ] || fail "NAME section is wrong: $page.md"

  if grep -n -E '^(===+|---+)[[:space:]]*$' "$source" >/dev/null; then
    fail "Setext heading or horizontal rule in $page.md"
  fi
  if grep -n -E "^[.'](TH|SH|SS|TP|IP|PP|RS|RE|EX|EE)([[:space:]]|$)" \
      "$source" >/dev/null; then
    fail "raw roff in $page.md"
  fi
  if grep -n -E '^[[:space:]]*</?[A-Za-z][^>]*>' "$source" >/dev/null; then
    fail "raw HTML in $page.md"
  fi
  if grep -n "$(printf '\t')" "$source" >/dev/null; then
    fail "tab in $page.md"
  fi
  if grep -n -E '[[:blank:]]+$' "$source" >/dev/null; then
    fail "trailing whitespace in $page.md"
  fi
}

check_page libpkgsource.3 \
  '% LIBPKGSOURCE(3) libpkgsource 4.1.0 | libpkgsource' \
  'libpkgsource - seal parser-neutral package-source authority'
check_page pkgsource_model.3 \
  '% PKGSOURCE_MODEL(3) libpkgsource 4.1.0 | libpkgsource' \
  'pkgsource_model - describe parser-neutral package-source value domains'
check_page pkgsource_profile.3 \
  '% PKGSOURCE_PROFILE(3) libpkgsource 4.1.0 | libpkgsource' \
  'pkgsource_profile - seal authoritative requirement profiles'
check_page pkgsource_recipe.3 \
  '% PKGSOURCE_RECIPE(3) libpkgsource 4.1.0 | libpkgsource' \
  'pkgsource_recipe - seal normalized native recipe authority'
check_page pkgsource_snapshot.3 \
  '% PKGSOURCE_SNAPSHOT(3) libpkgsource 4.1.0 | libpkgsource' \
  'pkgsource_snapshot - seal immutable package-source authority'
check_page pkgsource_codec.3 \
  '% PKGSOURCE_CODEC(3) libpkgsource 4.1.0 | libpkgsource' \
  'pkgsource_codec - encode canonical durable source-authority records'

for page in "$root"/docs/man/*.md; do
  grep -F '```cpp' "$page" >/dev/null ||
    fail "SYNOPSIS is not a C++ fenced block: ${page##*/}"
done
