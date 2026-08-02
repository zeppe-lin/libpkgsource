#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail()
{
  echo "test-structure: $*" >&2
  exit 1
}

for directory in codec internal model profiles public recipes support; do
  [ -d "$root/tests/$directory" ] || fail "missing tests/$directory"
done

for obsolete in \
  tests/model/value_domains_test.cpp \
  tests/profiles/sealing_test.cpp \
  tests/recipes/sealing_test.cpp \
  tests/codec/codec_test.cpp; do
  [ ! -e "$root/$obsolete" ] || fail "monolithic test remains: $obsolete"
done

for purpose in \
  codec-golden-vectors codec-profile-catalog codec-source-snapshot \
  codec-envelope-failures codec-identity-failures codec-value-failures \
  codec-canonicality profile-expansion profile-requirements recipe-identity \
  recipe-failures sha256-provider; do
  grep -F "'$purpose'" "$root/tests/meson.build" >/dev/null ||
    fail "missing attributable test: $purpose"
done
