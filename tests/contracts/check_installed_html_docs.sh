#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
prefix=${1:?installation prefix required}
version=${2:?project version required}
root=$prefix/share/htmldocs/libpkgsource/$version
fail()
{
  echo "installed-html-docs: $*" >&2
  exit 1
}
for file in \
  index.html architecture.html abi.html testing.html \
  manual/libpkgsource.3.html manual/pkgsource_model.3.html \
  manual/pkgsource_profile.3.html manual/pkgsource_recipe.3.html \
  manual/pkgsource_snapshot.3.html manual/pkgsource_codec.3.html \
  protocols/source-records-v1.html history/3.0-authority-reset.html \
  api/index.html assets/house.css legal/COPYING legal/COPYRIGHT
do
  [ -s "$root/$file" ] || fail "missing installed HTML artifact: $file"
done
if find "$root" -type f -name '*.md' | grep . >/dev/null; then
  fail 'canonical Markdown escaped into the rendered HTML tree'
fi
