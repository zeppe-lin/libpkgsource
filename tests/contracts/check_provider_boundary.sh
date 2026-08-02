#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail()
{
  echo "provider-boundary: $*" >&2
  exit 1
}

provider=$root/internal/sha256_openssl.cpp
[ -f "$provider" ] || fail 'OpenSSL provider translation unit is missing'
grep -F '#include <openssl/evp.h>' "$provider" >/dev/null ||
  fail 'OpenSSL provider does not own EVP integration'

matches=$(grep -R -l -E 'openssl/|EVP_' \
  "$root/src" "$root/codec" "$root/internal" "$root/include" || true)
[ "$matches" = "$provider" ] || {
  printf '%s\n' "$matches" >&2
  fail 'OpenSSL vocabulary escaped the qualified provider'
}

for consumer in "$root/src/meson.build" "$root/codec/meson.build"; do
  grep -F 'libpkgsource_sha256_provider_dep' "$consumer" >/dev/null ||
    fail "provider dependency is missing from ${consumer#$root/}"
done
