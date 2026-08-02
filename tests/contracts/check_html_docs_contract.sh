#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=$1

fail()
{
  echo "html-docs-contract-test: $*" >&2
  exit 1
}

for file in \
  docs/html.md \
  docs/assets/house.css \
  docs/assets/doxygen-extra.css \
  tools/build-html-docs.py \
  tools/check-html-docs.py \
  tools/install-html-docs.py
do
  [ -s "$root/$file" ] || fail "$file is missing or empty"
done

for script in build-html-docs.py check-html-docs.py install-html-docs.py; do
  [ -x "$root/tools/$script" ] || fail "tools/$script is not executable"
done

grep -F "'html_docs'" "$root/meson.options" >/dev/null ||
  fail 'html_docs option is missing'
grep -F "value: 'disabled'" "$root/meson.options" >/dev/null ||
  fail 'HTML generation is not disabled by default'
grep -F 'custom_target(' "$root/docs/meson.build" >/dev/null ||
  fail 'HTML documentation has no build target'
grep -F 'build_by_default: true' "$root/docs/meson.build" >/dev/null ||
  fail 'enabled HTML documentation is not part of the build graph'
grep -F "install_tag: 'html-docs'" "$root/docs/meson.build" >/dev/null ||
  fail 'HTML documentation has no dedicated install tag'
grep -F 'MESON_INSTALL_DESTDIR_PREFIX' "$root/tools/install-html-docs.py" >/dev/null ||
  fail 'HTML installer does not honor Meson DESTDIR staging'
grep -F 'GENERATE_HTML = YES' "$root/tools/build-html-docs.py" >/dev/null ||
  fail 'Doxygen HTML generation is not enabled explicitly'
grep -F "include/libpkgsource-codec" "$root/tools/build-html-docs.py" >/dev/null ||
  fail 'Doxygen HTML omits the codec public API'
grep -F -- '--fail-if-warnings' "$root/tools/build-html-docs.py" >/dev/null ||
  fail 'Pandoc warnings are not fatal for HTML generation'
