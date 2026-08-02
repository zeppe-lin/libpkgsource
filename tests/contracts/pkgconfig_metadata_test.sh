#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
build_root=$1
project_version=$2
fail()
{
  echo "pkgconfig-metadata-test: $*" >&2
  exit 1
}
find_pc()
{
  name=$1
  file=$build_root/meson-private/$name.pc
  [ -s "$file" ] || file=$(find "$build_root" -type f -name "$name.pc" -print | sed -n '1p')
  [ -n "${file:-}" ] && [ -s "$file" ] || fail "generated $name.pc not found"
  printf '%s\n' "$file"
}
count_token()
{
  field=$1
  token=$2
  file=$3
  awk -v field="$field" -v token="$token" '
    $1 == field {
      for (i = 2; i <= NF; ++i)
        if ($i == token) ++count
    }
    END { print count + 0 }
  ' "$file"
}

count_requirement()
{
  field=$1
  module=$2
  file=$3
  awk -v field="$field" -v module="$module" '
    $1 == field {
      line = $0
      sub(/^[^:]+:[[:space:]]*/, "", line)
      requirement_count = split(line, requirements, /,[[:space:]]*/)
      for (item = 1; item <= requirement_count; ++item) {
        requirement = requirements[item]
        sub(/^[[:space:]]*/, "", requirement)
        if (requirement ~ ("^" module "([[:space:]]|$)"))
          ++matches
      }
    }
    END { print matches + 0 }
  ' "$file"
}

core=$(find_pc libpkgsource)
codec=$(find_pc libpkgsource-codec)

grep -F 'Name: libpkgsource' "$core" >/dev/null
grep -F "Version: $project_version" "$core" >/dev/null
grep -E 'Libs:.*-lpkgsource([[:space:]]|$)' "$core" >/dev/null
[ "$(count_token Requires.private: libcrypto "$core")" -eq 1 ] ||
  fail 'core metadata must contain one private libcrypto requirement'
if grep -E '^Requires:.*libpkgsource-codec' "$core" >/dev/null; then
  fail 'core metadata depends on codec'
fi

grep -F 'Name: libpkgsource-codec' "$codec" >/dev/null
grep -F "Version: $project_version" "$codec" >/dev/null
grep -E 'Libs:.*-lpkgsource-codec([[:space:]]|$)' "$codec" >/dev/null
public_core_count=$(count_requirement Requires: libpkgsource "$codec")
[ "$public_core_count" -eq 1 ] ||
  fail "codec metadata must contain one public core requirement, found $public_core_count"
grep -E "^Requires:.*libpkgsource[[:space:]]*=[[:space:]]*$project_version([,[:space:]]|$)" \
  "$codec" >/dev/null || fail 'codec metadata does not require the exact core version'
private_core_count=$(count_requirement Requires.private: libpkgsource "$codec")
[ "$private_core_count" -eq 0 ] ||
  fail "codec metadata must not repeat core privately, found $private_core_count"
if grep -E '^Libs.private:.*-lpkgsource([[:space:]]|$)' "$codec" >/dev/null; then
  fail 'codec metadata must not repeat core in Libs.private'
fi
[ "$(count_token Requires.private: libcrypto "$codec")" -eq 1 ] ||
  fail 'codec metadata must contain one private libcrypto requirement'
