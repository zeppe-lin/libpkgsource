#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

tool=$1
worker=$2
source_dir=$3
one=${MESON_BUILD_ROOT:-/tmp}/pkgsource-blackbox-one.$$
two=${MESON_BUILD_ROOT:-/tmp}/pkgsource-blackbox-two.$$
trap 'rm -f "$one" "$two"' EXIT HUP INT TERM
"$tool" --worker "$worker" "$source_dir" > "$one"
"$tool" --worker "$worker" "$source_dir" > "$two"
cmp "$one" "$two"
grep -q '"schema":"pkgsource-inspect/1"' "$one"
grep -q '"format":"pkgfile/0"' "$one"
grep -q '"name":"demo"' "$one"
