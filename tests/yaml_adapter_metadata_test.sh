#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
build_root=$1
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
grep -E 'Requires:.*libpkgsource[[:space:]]*=[[:space:]]*1\.1\.0' "$metadata" >/dev/null
grep -E \
  'Requires.private:.*yaml-0\.1[[:space:]]*>=[[:space:]]*0\.2\.5' \
  "$metadata" >/dev/null
grep -E 'Libs:.*-lpkgsource-yaml' "$metadata" >/dev/null
