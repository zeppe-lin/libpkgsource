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

for directory in unit integration protocol mechanism header installed support contracts; do
  [ -d "$root/tests/$directory" ] || fail "missing tests/$directory"
done

for obsolete in model profiles recipes codec internal public; do
  [ ! -e "$root/tests/$obsolete" ] || fail "obsolete tests/$obsolete remains"
done

for purpose in \
  value-admission sealed-profile sealed-recipe source-authority source-identity \
  codec-golden-vectors codec-profile-catalog codec-source-snapshot \
  codec-envelope-failures codec-identity-failures codec-value-failures \
  codec-canonicality profile-expansion profile-requirements recipe-failures \
  sha256-provider; do
  grep -F "'$purpose'" "$root/tests/meson.build" >/dev/null ||
    fail "missing attributable test: $purpose"
done

for suite in unit integration protocol mechanism header contracts; do
  grep -F "suite: '$suite'" "$root/tests/meson.build" >/dev/null ||
    fail "missing Meson suite: $suite"
done

for consumer in core_consumer.cpp codec_consumer.cpp; do
  [ -f "$root/tests/installed/$consumer" ] ||
    fail "missing installed consumer: $consumer"
done
