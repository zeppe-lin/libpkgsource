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
require "$root/meson.build" "  version: '1.0.0',"
require "$root/src/meson.build" "  soversion: '1',"
require "$root/src/meson.build" "'../include/libpkgsource/identity.h'"
require "$root/src/meson.build" "'../include/libpkgsource/model.h'"
