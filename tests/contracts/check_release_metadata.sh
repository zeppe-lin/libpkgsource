#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=$1

fail()
{
  echo "release-metadata-test: $*" >&2
  exit 1
}

require()
{
  file=$1
  text=$2
  grep -F -- "$text" "$file" >/dev/null ||
    fail "${file#$root/} omits: $text"
}

require "$root/meson.build" "  version: '3.0.0',"
require "$root/src/meson.build" "  soversion: '3',"
require "$root/codec/meson.build" "  soversion: '1',"
require "$root/HISTORY.md" '## 3.0.0'
require "$root/src/meson.build" "'../include/libpkgsource/profile.h'"
require "$root/src/meson.build" "'../include/libpkgsource/recipe.h'"
require "$root/src/meson.build" "'../include/libpkgsource/snapshot.h'"
require "$root/codec/meson.build" "'../include/libpkgsource-codec/libpkgsource-codec.h'"
require "$root/include/libpkgsource/recipe.h" 'check_program() const noexcept'
require "$root/codec/meson.build" 'requires: [libpkgsource_dep]'
require "$root/src/snapshot.cpp" 'libpkgsource/source-snapshot/v1'
require "$root/docs/protocols/source-records-v1.md" 'schema 1'
require "$root/abi/libpkgsource.exports" '_ZN9pkgsource11seal_source'
for operation in encode_profile_catalog decode_profile_catalog \
                 encode_source_snapshot decode_source_snapshot
do
  grep -F "$operation" "$root/include/libpkgsource-codec/codec.h" >/dev/null ||
    fail "codec header omits public operation: $operation"
done
[ "$(wc -l < "$root/abi/libpkgsource-codec.exports")" -eq 13 ] ||
  fail 'codec ABI manifest does not bind all public operations and error symbols'

if grep -R -E 'libpkgsource-(yaml|plan)|yaml_adapter|plan_adapter|source_syntax|recipe_identity' \
    "$root/meson.build" "$root/meson.options" "$root/src" "$root/internal" \
    "$root/include/libpkgsource" >/dev/null; then
  fail 'split adapter or removed-generation contamination remains in core'
fi
