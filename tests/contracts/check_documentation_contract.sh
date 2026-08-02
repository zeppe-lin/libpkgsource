#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=$1

fail()
{
  echo "documentation-contract-test: $*" >&2
  exit 1
}

require_heading()
{
  file=$1
  heading=$2
  grep -F -- "$heading" "$file" >/dev/null ||
    fail "${file#$root/} omits heading: $heading"
}

for document in \
  README.md \
  HISTORY.md \
  CONTRIBUTING.md \
  MAINTAINING.md \
  docs/architecture.md \
  docs/abi.md \
  docs/code-style.md \
  docs/testing.md \
  docs/manpage-markdown.md \
  docs/html.md \
  docs/history/3.0-authority-reset.md \
  docs/protocols/source-records-v1.md
do
  [ -s "$root/$document" ] || fail "$document is missing or empty"
done

require_heading "$root/README.md" '# libpkgsource'
require_heading "$root/README.md" '## Public libraries'
require_heading "$root/README.md" '## Authority boundary'
require_heading "$root/README.md" '## Installed documentation'
require_heading "$root/README.md" '## HTML documentation'
grep -F 'Meson 1.9 or newer is required' "$root/README.md" >/dev/null ||
  fail 'README omits the actual Meson feature floor'
require_heading "$root/docs/architecture.md" '## Authority boundary'
require_heading "$root/docs/architecture.md" '## Repository layout'
require_heading "$root/docs/architecture.md" '## Semantic pipeline'
require_heading "$root/docs/architecture.md" '## SHA-256 provider boundary'
require_heading "$root/docs/architecture.md" '## Codec ownership'
require_heading "$root/docs/architecture.md" '## Decode discipline'
require_heading "$root/docs/architecture.md" '## Installed documentation'
require_heading "$root/docs/architecture.md" '## HTML publication boundary'
require_heading "$root/docs/abi.md" '## Canonical manifests'
require_heading "$root/docs/abi.md" '## Versioning'
require_heading "$root/docs/abi.md" '## Qualification'
require_heading "$root/docs/testing.md" '## Core behavior'
require_heading "$root/docs/testing.md" '## Codec behavior'
require_heading "$root/docs/testing.md" '## Release qualification'
require_heading "$root/docs/manpage-markdown.md" '## Conversion contract'
require_heading "$root/docs/manpage-markdown.md" '## Forbidden Markdown'
require_heading "$root/docs/html.md" '## Output and installation'
require_heading "$root/docs/html.md" '## Validation'
require_heading "$root/CONTRIBUTING.md" '## Boundary first'
require_heading "$root/CONTRIBUTING.md" '## Identity and record changes'
require_heading "$root/MAINTAINING.md" '## Release checklist'
require_heading "$root/HISTORY.md" '## 3.0.0'

for manual in \
  libpkgsource.3 \
  pkgsource_model.3 \
  pkgsource_profile.3 \
  pkgsource_recipe.3 \
  pkgsource_snapshot.3 \
  pkgsource_codec.3
do
  [ -s "$root/docs/man/$manual.md" ] || fail "missing canonical manual: $manual.md"
  [ -s "$root/docs/man/generated/$manual" ] || fail "missing generated manual: $manual"
done

grep -F 'Input syntax is not authority' "$root/docs/architecture.md" >/dev/null ||
  fail 'architecture omits the declaration-to-authority distinction'
grep -F 'libpkgsource/source-snapshot/v1' "$root/docs/architecture.md" >/dev/null ||
  fail 'architecture omits the source identity domain'
grep -F 'abi/libpkgsource.exports' "$root/docs/abi.md" >/dev/null ||
  fail 'ABI policy omits the core manifest'
grep -F 'abi/libpkgsource-codec.exports' "$root/docs/abi.md" >/dev/null ||
  fail 'ABI policy omits the codec manifest'
grep -F 'tests/codec/golden_vectors_test.cpp' \
  "$root/docs/protocols/source-records-v1.md" >/dev/null ||
  fail 'record protocol omits its executable vector owner'

if grep -R -n '<!-- SPDX-' "$root"/*.md "$root/docs" >/dev/null; then
  fail 'Markdown contains SPDX comments; use COPYING and COPYRIGHT'
fi

if find "$root/docs/man" -maxdepth 1 -type f -name '*.scd' | grep . >/dev/null; then
  fail 'scdoc source remains after Markdown migration'
fi

for retired in DESIGN.md TESTING.md CODESTYLE.md MANPAGE-MARKDOWN.md \
               SOURCE-RECORDS-1.md MIGRATION.md
do
  [ ! -e "$root/$retired" ] || fail "retired root document remains: $retired"
done
