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
count=$(grep '^Requires:' "$codec" | grep -oE 'libpkgsource[[:space:]]*=[[:space:]]*[^,[:space:]]+' | wc -l)
[ "$count" -eq 1 ] || fail "codec metadata must contain one exact core requirement, found $count"
grep -E "^Requires:.*libpkgsource[[:space:]]*=[[:space:]]*$project_version([,[:space:]]|$)" \
  "$codec" >/dev/null || fail 'codec metadata does not require the exact core version'
[ "$(count_token Requires.private: libcrypto "$codec")" -eq 1 ] ||
  fail 'codec metadata must contain one private libcrypto requirement'
