#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=$1

fail()
{
  echo "codec-contract-test: $*" >&2
  exit 1
}

require()
{
  file=$1
  text=$2
  grep -F -- "$text" "$file" >/dev/null ||
    fail "${file#$root/} omits: $text"
}

header=$root/include/libpkgsource-codec/codec.h
profile_source=$root/codec/internal/profile_catalog_record.cpp
snapshot_source=$root/codec/internal/source_snapshot_record.cpp
record_source=$root/codec/internal/record_io.cpp
spec=$root/docs/protocols/source-records-v1.md

for file in "$header" "$profile_source" "$snapshot_source" "$record_source" "$spec"; do
  [ -s "$file" ] || fail "missing ${file#$root/}"
done

require "$header" 'namespace pkgsource::codec'
require "$header" 'profile_catalog_encoding_version = 1'
require "$header" 'source_snapshot_encoding_version = 1'
require "$header" 'maximum_profile_catalog_encoding_size'
require "$header" 'maximum_source_snapshot_encoding_size'
require "$header" 'encode_profile_catalog('
require "$header" 'decode_profile_catalog('
require "$header" 'encode_source_snapshot('
require "$header" 'decode_source_snapshot('
require "$profile_source" 'profile_catalog::seal('
require "$profile_source" 'encode_profile_catalog(result) != encoding'
require "$snapshot_source" 'seal_source('
require "$snapshot_source" 'encode_source_snapshot(result) != encoding'
require "$record_source" 'checksum_mismatch'
require "$root/codec/meson.build" "soversion: '1'"
require "$root/codec/meson.build" "requires: [libpkgsource_dep]"
require "$spec" 'byte[8] magic = "ZLPSPCAT"'
require "$spec" 'byte[8] magic = "ZLPSSNAP"'

if grep -R -E 'source_syntax|recipe_yaml_v[0-9]|recipe_identity' \
    "$root/include/libpkgsource-codec" "$root/codec" >/dev/null; then
  fail 'codec contains parser generations or removed identities'
fi

if grep -R -E 'yaml_parser|libpkgplan|pkgcatalog' \
    "$root/include/libpkgsource-codec" "$root/codec" >/dev/null; then
  fail 'codec imports syntax, planner, or catalog authority'
fi
