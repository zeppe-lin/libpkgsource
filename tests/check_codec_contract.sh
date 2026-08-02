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
header=$root/include/libpkgsource-codec/codec.h
source=$root/codec/codec.cpp
require "$header" 'namespace pkgsource::codec'
require "$header" 'profile_catalog_encoding_version = 1'
require "$header" 'source_snapshot_encoding_version = 1'
require "$header" 'maximum_profile_catalog_encoding_size'
require "$header" 'maximum_source_snapshot_encoding_size'
require "$header" 'encode_profile_catalog('
require "$header" 'decode_profile_catalog('
require "$header" 'encode_source_snapshot('
require "$header" 'decode_source_snapshot('
require "$source" 'profile_catalog::seal('
require "$source" 'seal_source('
require "$source" 'encode_profile_catalog(result) != encoding'
require "$source" 'encode_source_snapshot(result) != encoding'
require "$source" 'checksum_mismatch'
require "$root/codec/meson.build" "soversion: '1'"
require "$root/codec/meson.build" "'libpkgsource = ' + meson.project_version()"

if grep -R -E 'source_syntax|recipe_yaml_v[0-9]|recipe_identity' \
    "$header" "$source" >/dev/null; then
  echo 'codec record still contains parser generations or removed identities' >&2
  exit 1
fi
