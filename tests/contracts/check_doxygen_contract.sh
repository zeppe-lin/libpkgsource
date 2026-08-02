#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=${1:?source root required}
fail()
{
  echo "doxygen-contract: $*" >&2
  exit 1
}

for setting in \
  'EXTRACT_ALL            = NO' \
  'WARN_IF_UNDOCUMENTED   = YES' \
  'WARN_IF_DOC_ERROR      = YES' \
  'WARN_NO_PARAMDOC       = YES' \
  'WARN_AS_ERROR          = YES'; do
  grep -F "$setting" "$root/Doxyfile" >/dev/null ||
    fail "missing strict setting: $setting"
done

grep -F 'INPUT                  = include/libpkgsource include/libpkgsource-codec' \
  "$root/Doxyfile" >/dev/null || fail 'both public APIs must be documented'

for header in "$root"/include/libpkgsource/*.h \
              "$root"/include/libpkgsource-codec/*.h; do
  case $header in
    */export.h)
      continue
      ;;
  esac
  grep -F '@file' "$header" >/dev/null ||
    fail "missing file documentation: ${header#$root/}"
done

for token in \
  'namespace pkgsource' \
  'namespace pkgsource::codec' \
  '@param code' \
  '@param document' \
  '@param declaration' \
  '@param encoding' \
  '@return'; do
  grep -R -F "$token" "$root/include" >/dev/null ||
    fail "missing public documentation token: $token"
done

! grep -R -F 'PKGSOURCE_DECLARE_IDENTITY' "$root/include" >/dev/null ||
  fail 'public identity declarations must remain explicit and documentable'
