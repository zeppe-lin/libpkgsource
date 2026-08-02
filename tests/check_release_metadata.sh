#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
require() {
  file=$1
  text=$2
  grep -F -- "$text" "$file" >/dev/null || {
    echo "missing release metadata in ${file#$root/}: $text" >&2
    exit 1
  }
}
require "$root/meson.build" "  version: '3.0.0',"
require "$root/src/meson.build" "  soversion: '3',"
require "$root/HISTORY.md" '## 2.1.0'
require "$root/src/meson.build" "'../include/libpkgsource/profile.h'"
require "$root/src/meson.build" "'../include/libpkgsource/recipe.h'"
require "$root/src/meson.build" "'../include/libpkgsource/snapshot.h'"
require "$root/include/libpkgsource/recipe.h" "check_program() const noexcept"
require "$root/codec/meson.build" "  soversion: '1',"
require "$root/codec/meson.build" "'libpkgsource = ' + meson.project_version()"

if grep -R -E 'libpkgsource-(yaml|plan)|yaml_adapter|planner_adapter' \
    "$root/meson.build" "$root/meson.options" "$root/src" \
    "$root/include/libpkgsource" >/dev/null; then
  echo 'split adapter contamination remains in core build or headers' >&2
  exit 1
fi
