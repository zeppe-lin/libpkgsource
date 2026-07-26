#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
src=${1:-.}
work=${2:-"$src/.linkage-matrix"}
rm -rf "$work"

for mode in shared static; do
  build="$work/$mode"
  prefix="$work/prefix-$mode"

  meson setup "$build" "$src" \
    --prefix="$prefix" \
    -Ddefault_library="$mode" \
    -Dlink_mode="$mode" \
    -Dreference_tools=disabled
  meson compile -C "$build"
  meson test -C "$build" --print-errorlogs
  meson install -C "$build"

  pc=$(find "$prefix" -type f -name libpkgsource.pc -print -quit)
  [ -n "$pc" ] || {
    echo "$mode: installed libpkgsource.pc not found" >&2
    exit 1
  }
  pcdir=$(dirname "$pc")
  worker=$(PKG_CONFIG_PATH="$pcdir" \
    pkg-config --variable=pkgfile_worker libpkgsource)
  expected="$prefix/libexec/pkgsource-pkgfile-worker"

  [ "$worker" = "$expected" ] || {
    echo "$mode: worker variable is '$worker', expected '$expected'" >&2
    exit 1
  }
  [ -x "$worker" ] || {
    echo "$mode: installed worker is missing or not executable: $worker" >&2
    exit 1
  }
done
