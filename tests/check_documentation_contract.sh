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
require()
{
  file=$1
  text=$2
  grep -F -- "$text" "$file" >/dev/null ||
    fail "${file#$root/} omits: $text"
}

require "$root/README.md" 'libpkgsource-codec'
require "$root/DESIGN.md" 'syntax adapter'
require "$root/DESIGN.md" 'libpkgsource-codec.so.1'
require "$root/MIGRATION.md" 'libpkgsource-yaml'
require "$root/MIGRATION.md" 'libpkgsource-plan'
require "$root/TESTING.md" 'golden vectors'
require "$root/SOURCE-RECORDS-1.md" 'ZLPSPCAT'
require "$root/SOURCE-RECORDS-1.md" 'ZLPSSNAP'
require "$root/SOURCE-RECORDS-1.md" '2f268947090f17e2c4f1825c0c7167930c8950327c6389c6531d8e6f64b4e483'
require "$root/SOURCE-RECORDS-1.md" 'cd221e9527162de41fa23806f2a370e161139cf059c6dc77d08cbfd37b45be35'
require "$root/man/pkgsource_codec.3.scd" 'SOURCE-RECORDS-1.md'

for page in \
  libpkgsource.3.scd \
  pkgsource_model.3.scd \
  pkgsource_profile.3.scd \
  pkgsource_recipe.3.scd \
  pkgsource_snapshot.3.scd \
  pkgsource_codec.3.scd
do
  [ -f "$root/man/$page" ] || fail "missing man/$page"
done
