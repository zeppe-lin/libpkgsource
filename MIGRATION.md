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

Consumers must move atomically to SONAME 2 and the matching headers.  The
optional YAML and planner adapters move with the core because their public C++
values contain recipe or source-snapshot authority.  There is no supported
binary compatibility bridge inside `libpkgsource`.


## Native syntax transition

Version 1.1 adds `libpkgsource-yaml`; it does not add Pkgfile compatibility.
Migration tools may emit `profiles.yml/1` and `recipe.yml/1`, after which the
strict adapter and native sealers validate the result.  Ambiguous historical
semantics remain migration errors or explicit operator decisions.

The YAML adapter accepts document bytes only.  Historical collection layout,
`pkgman.conf`, directory naming, and source-tree precedence are not parser
semantics and remain outside this repository.

## Native check-program transition

Version 2 adds `recipe.yml/2` with one optional exact check program.
`recipe.yml/1` remains accepted unchanged and does not acquire executable check
authority.  Migration tools that can recover an unambiguous historical check
function may emit version two; ambiguous historical behavior remains an
operator decision rather than a core inference.

Check execution paths, environment, success policy, and terminal evidence are
not migration semantics and remain outside this repository.

## Durable source evidence

Version 2.1 adds native durable encodings; it does not turn historical input
into native authority.  Migration tools may first produce valid native profile
and recipe declarations and pass them through the ordinary sealers.  Only the
resulting sealed profile catalog and source snapshot may then be encoded.

The decoder never reparses historical Pkgfiles or YAML documents and does not
search for replacement source trees when a record is absent.  Missing durable
source authority remains a missing-authority error for the owning orchestration
layer, not an invitation to infer semantics from whatever files happen to be
present.
