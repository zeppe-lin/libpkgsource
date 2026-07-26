<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Migration boundary

The native `libpkgsource` API is not a compatibility layer.

It will not implement:

* `Pkgfile/0` parsing or shell evaluation;
* CRUX metadata-comment or sidecar semantics;
* a `build_and_run` dependency scope;
* pkgmk or pkgman behavioral emulation; or
* inference of native package semantics from historical source files.

Historical sources will be converted by a separate tool, provisionally named
`Pkgfile-to-recipe.yml`.  That tool may contain compatibility policy and may
require operator decisions when old declarations are ambiguous.  Its output is
then validated as native input; migration behavior does not become part of the
library ABI.

The same rule applies downstream.  Historical package database import belongs
to a separate `legacy-db-to-canonical-state` tool, fakeroot compatibility does
not belong in `libpkgbuild`, and pkgmk/pkgman behavioral compatibility does not
belong in `pkgctl`.

## Downstream transition

`libpkgsource-plan` is rebuilt with this repository because it is an explicit
adapter over the public source model.  It must transition from the removed
legacy dependency and captured-file APIs to exact native run requirements and
inline sealed lifecycle programs.

`libpkgplan` needs no source change for the initial transition.  Its runtime
dependency declarations, removal lifecycle declarations, and target-profile
facts can receive the native projection.  It remains deliberately unaware of
build requirements, check requirements, lifecycle requirements, profile
expansion, source inputs, and build execution.

Consumers must move atomically to the new SONAME and headers.  There is no
supported source or binary compatibility bridge inside `libpkgsource`.
