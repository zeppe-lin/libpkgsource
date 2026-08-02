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
  include/libpkgsource/libpkgsource.h \
  include/libpkgsource/snapshot.h \
  include/libpkgsource-codec/libpkgsource-codec.h \
  docs/protocols/source-records-v1.md
do
  [ -s "$root/$required" ] || fail "missing $required"
done

if grep -R -E 'yaml(-0\.1)?|libpkgplan|source_syntax|recipe_identity|recipe_yaml_v[0-9]' \
    "$root/src" "$root/internal" "$root/include/libpkgsource" \
    "$root/meson.build" "$root/src/meson.build" >/dev/null; then
  fail 'semantic core contains syntax, planner, or removed identity vocabulary'
fi

if grep -F 'libpkgsource-codec' "$root/include/libpkgsource/libpkgsource.h" >/dev/null; then
  fail 'core umbrella header imports the separately linked codec surface'
fi

grep -F 'seal_source(source_origin origin,' \
  "$root/include/libpkgsource/snapshot.h" >/dev/null ||
  fail 'source sealer signature is missing'
grep -F 'libpkgsource/source-snapshot/v1' "$root/src/snapshot.cpp" >/dev/null ||
  fail 'source-snapshot identity domain is missing'
grep -F 'check requirements without check program' "$root/src/recipe.cpp" >/dev/null ||
  fail 'check requirement/program closure is not enforced by core'

grep -F "gnu_symbol_visibility: 'hidden'" "$root/src/meson.build" >/dev/null ||
  fail 'core shared-library visibility is not hidden by default'
grep -F 'abi/libpkgsource.exports' "$root/docs/abi.md" >/dev/null ||
  fail 'core ABI manifest is not documented'
