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
require "$root/meson.build" "  version: '0.2.0',"
require "$root/src/meson.build" "  soversion: '0',"
require "$root/adapter/meson.build" "  soversion: '0',"
require "$root/adapter/meson.build" "'libpkgplan >= 0.2.0'"
require "$root/HISTORY.md" '## 0.2.0'
require "$root/README.md" 'Version 0.2.0'
require "$root/man/libpkgsource.3.scd" 'Version 0.2.0'
