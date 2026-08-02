#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=$1
build=$root/docs/meson.build
man_build=$root/docs/man/meson.build

fail()
{
  echo "documentation-install-contract-test: $*" >&2
  exit 1
}

[ -s "$build" ] || fail 'docs/meson.build is missing'
[ -s "$man_build" ] || fail 'docs/man/meson.build is missing'

grep -F "get_option('datadir') / 'doc' / meson.project_name()" "$build" >/dev/null ||
  fail 'canonical documentation install root is not project-owned'
grep -F "install_tag: 'doc'" "$build" >/dev/null ||
  fail 'canonical documentation lacks the doc install tag'
grep -F "install_tag: 'man'" "$man_build" >/dev/null ||
  fail 'generated manuals lack the man install tag'
grep -F "install_tag: 'html-docs'" "$build" >/dev/null ||
  fail 'rendered HTML lacks the html-docs install tag'

for source in \
  'man/libpkgsource.3.md' \
  'man/pkgsource_model.3.md' \
  'man/pkgsource_profile.3.md' \
  'man/pkgsource_recipe.3.md' \
  'man/pkgsource_snapshot.3.md' \
  'man/pkgsource_codec.3.md' \
  'protocols/source-records-v1.md' \
  'history/3.0-authority-reset.md' \
  'assets/house.css'
do
  grep -F "'$source'" "$build" >/dev/null ||
    fail "canonical documentation install omits $source"
done

if grep -F 'install_subdir(' "$build" >/dev/null; then
  fail 'documentation installation copies an unreviewed source subtree'
fi

for script in check_installed_docs.sh check_installed_html_docs.sh; do
  [ -x "$root/tests/contracts/$script" ] || fail "missing executable $script"
done
