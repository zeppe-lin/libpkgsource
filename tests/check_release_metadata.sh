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
require "$root/HISTORY.md" '## 3.0.0 — unreleased'
require "$root/src/meson.build" "'../include/libpkgsource/profile.h'"
require "$root/src/meson.build" "'../include/libpkgsource/recipe.h'"
require "$root/src/meson.build" "'../include/libpkgsource/snapshot.h'"
require "$root/include/libpkgsource/recipe.h" 'check_program() const noexcept'
require "$root/codec/meson.build" "'libpkgsource = ' + meson.project_version()"
require "$root/src/snapshot.cpp" 'libpkgsource/source-snapshot/v1'

if grep -R -E 'libpkgsource-(yaml|plan)|yaml_adapter|planner_adapter|source_syntax|recipe_identity' \
    "$root/meson.build" "$root/meson.options" "$root/src" \
    "$root/include/libpkgsource" >/dev/null; then
  fail 'split adapter or removed generation contamination remains in core'
fi
