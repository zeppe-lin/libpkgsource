#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
require() {
  file=$1
  text=$2
  grep -F -- "$text" "$file" >/dev/null || {
    echo "missing codec contract in ${file#$root/}: $text" >&2
    exit 1
  }
}
require_absent() {
  file=$1
  text=$2
  if grep -F -- "$text" "$file" >/dev/null; then
    echo "forbidden codec dependency in ${file#$root/}: $text" >&2
    exit 1
  fi
}
require "$root/include/libpkgsource/codec.h" \
  'profile_catalog_encoding_version = 1'
require "$root/include/libpkgsource/codec.h" \
  'source_snapshot_encoding_version = 1'
require "$root/include/libpkgsource/codec.h" \
  'encode_profile_catalog('
require "$root/include/libpkgsource/codec.h" \
  'decode_profile_catalog('
require "$root/include/libpkgsource/codec.h" \
  'encode_source_snapshot('
require "$root/include/libpkgsource/codec.h" \
  'decode_source_snapshot('
require "$root/src/codec.cpp" \
  "'Z', 'L', 'P', 'S', 'P', 'C', 'A', 'T'"
require "$root/src/codec.cpp" \
  "'Z', 'L', 'P', 'S', 'S', 'N', 'A', 'P'"
require "$root/src/codec.cpp" 'profile_catalog::seal('
require "$root/src/codec.cpp" 'seal_source('
require "$root/src/codec.cpp" 'encode_profile_catalog(result) != encoding'
require "$root/src/codec.cpp" 'encode_source_snapshot(result) != encoding'
require_absent "$root/src/codec.cpp" 'libpkgsource-yaml'
require_absent "$root/src/codec.cpp" '<filesystem>'
require_absent "$root/src/codec.cpp" 'std::ifstream'
require_absent "$root/src/codec.cpp" 'std::ofstream'
require "$root/src/meson.build" "'codec.cpp'"
require "$root/src/meson.build" "'../include/libpkgsource/codec.h'"
require "$root/tests/meson.build" "'codec_test.cpp'"
require "$root/man/meson.build" \
  "['pkgsource_codec.3.scd', 'pkgsource_codec.3']"
for path in \
  include/libpkgsource/codec.h \
  src/codec.cpp \
  tests/codec_test.cpp \
  man/pkgsource_codec.3.scd
do
  test -f "$root/$path" || {
    echo "missing codec path: $path" >&2
    exit 1
  }
done
