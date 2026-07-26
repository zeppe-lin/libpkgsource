#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=${1:-.}

require() {
  grep -F -- "$2" "$root/$1" >/dev/null || {
    echo "$1: missing worker metadata contract: $2" >&2
    exit 1
  }
}

require src/meson.build "'pkgfile_worker': pkgfile_worker_build_path"
require src/meson.build "'pkgfile_worker': pkgfile_worker_install_path"
require tests/meson.build "libpkgsource_dep.get_variable("
require tests/meson.build "internal: 'pkgfile_worker'"
require tests/meson.build "pkgconfig: 'pkgfile_worker'"
require tests/run-linkage-matrix.sh '--variable=pkgfile_worker'
