#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
build_root=$1
project_version=$2
metadata=$build_root/meson-private/libpkgsource-yaml.pc
[ -s "$metadata" ] || metadata=$(
  find "$build_root" -type f -name libpkgsource-yaml.pc -print |
    sed -n '1p'
)
[ -n "${metadata:-}" ] && [ -s "$metadata" ] || {
  echo 'yaml-adapter-metadata-test: generated metadata not found' >&2
  exit 1
}
grep -F 'Name: libpkgsource-yaml' "$metadata" >/dev/null
awk -v expected="$project_version" '
  $1 == "Requires:" {
    for (i = 2; i + 2 <= NF; ++i) {
      if ($i == "libpkgsource" && $(i + 1) == "=" &&
          $(i + 2) == expected) {
        found = 1
      }
    }
  }
  END { exit found ? 0 : 1 }
' "$metadata"
grep -E \
  'Requires.private:.*yaml-0\.1[[:space:]]*>=[[:space:]]*0\.2\.5' \
  "$metadata" >/dev/null
grep -E 'Libs:.*-lpkgsource-yaml' "$metadata" >/dev/null
