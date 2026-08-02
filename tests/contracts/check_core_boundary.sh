#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail()
{
  echo "core-boundary-test: $*" >&2
  exit 1
}

for required in \
  "$root/include/libpkgsource/libpkgsource.h" \
  "$root/include/libpkgsource/snapshot.h" \
  "$root/include/libpkgsource-codec/codec.h" \
  "$root/docs/protocols/source-records-v1.md"
do
  [ -f "$required" ] || fail "missing ${required#$root/}"
done

if grep -R -E 'yaml(-0\.1)?|libpkgplan|source_syntax|recipe_identity|recipe_yaml_v[0-9]' \
    "$root/src" "$root/include/libpkgsource" "$root/meson.build" \
    "$root/src/meson.build" >/dev/null; then
  fail 'semantic core contains syntax, planner, or removed identity vocabulary'
fi

if grep -F 'libpkgsource-codec/codec.h' \
    "$root/include/libpkgsource/libpkgsource.h" >/dev/null; then
  fail 'core umbrella header imports the optional codec surface'
fi

grep -F 'source_snapshot seal_source(' \
  "$root/include/libpkgsource/snapshot.h" >/dev/null ||
  fail 'source sealer signature is missing'
grep -F 'libpkgsource/source-snapshot/v1' "$root/src/snapshot.cpp" >/dev/null ||
  fail 'first public source identity domain is missing'
grep -F 'check requirements without check program' \
  "$root/src/recipe.cpp" >/dev/null ||
  fail 'check requirement/program closure is not enforced by core'
