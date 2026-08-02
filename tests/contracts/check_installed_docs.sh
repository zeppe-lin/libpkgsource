#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
prefix=${1:?installation prefix required}
docdir=$prefix/share/doc/libpkgsource
man3=$prefix/share/man/man3
fail()
{
  echo "installed-docs: $*" >&2
  exit 1
}

for file in \
  README.md HISTORY.md CONTRIBUTING.md MAINTAINING.md COPYING COPYRIGHT \
  architecture.md abi.md code-style.md testing.md manpage-markdown.md html.md \
  protocols/source-records-v1.md history/3.0-authority-reset.md \
  man/libpkgsource.3.md man/pkgsource_model.3.md \
  man/pkgsource_profile.3.md man/pkgsource_recipe.3.md \
  man/pkgsource_snapshot.3.md man/pkgsource_codec.3.md \
  assets/house.css assets/doxygen-extra.css
do
  [ -s "$docdir/$file" ] || fail "missing installed documentation: $file"
done

for page in libpkgsource.3 pkgsource_model.3 pkgsource_profile.3 \
            pkgsource_recipe.3 pkgsource_snapshot.3 pkgsource_codec.3
do
  [ -s "$man3/$page" ] || fail "missing installed manual: $page"
done

if find "$docdir" -type f \( -name meson.build -o -path '*/generated/*' \) |
    grep . >/dev/null; then
  fail 'build metadata or derived roff escaped into canonical documentation'
fi
