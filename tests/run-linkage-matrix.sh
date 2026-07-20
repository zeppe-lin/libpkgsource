#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
src=${1:-.}
work=${2:-"$src/.linkage-matrix"}
rm -rf "$work"
meson setup "$work/shared" "$src" -Ddefault_library=shared -Dlink_mode=shared -Dreference_tools=disabled
meson compile -C "$work/shared"
meson test -C "$work/shared" --print-errorlogs
meson setup "$work/static" "$src" -Ddefault_library=static -Dlink_mode=static -Dreference_tools=disabled
meson compile -C "$work/static"
meson test -C "$work/static" --print-errorlogs
