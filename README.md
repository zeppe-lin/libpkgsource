<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# libpkgsource

`libpkgsource` is the native Zeppe-Lin C++17 package-source authority.

It validates parser-neutral declarations, seals authoritative requirement
profiles, expands exact package requirements, and returns immutable normalized
recipe and source-snapshot values.  Input syntax is provenance.  The sealed
`source_snapshot` is authority.

Version 1 is intentionally incompatible with the historical implementation.  It
contains no Pkgfile backend, no CRUX package-source compatibility model, no MD5
source declarations, no sidecar semantics, and no combined `build_and_run`
requirement scope.  Historical conversion belongs to a separate migration tool.

## Native model

The normalized snapshot distinguishes:

* package release and package metadata;
* remote and local source input declarations with SHA-256 identities;
* exact build program material;
* build, run, check, and action-bound lifecycle requirements;
* exact package and named profile subjects;
* sealed selected build profiles and their complete transitive closure;
* installation and removal lifecycle programs;
* independent build and target architecture requirements; and
* profile, package-release, recipe, and source-snapshot identity domains.

Profiles are authoritative values rather than parser aliases.  Their names,
direct members, declaration provenance, deterministic expansion paths, nested
profile identities, and semantic identities remain available after sealing.
Unknown profiles, duplicate definitions or members, and cycles are rejected.

`check` is already a native typed requirement scope.  This reserves the test
requirement domain without adding test execution to the library.

## Input syntax

The initial syntax contract is [RECIPE-YAML-1.md](RECIPE-YAML-1.md).
`libpkgsource` does not contain a YAML parser in version 1.  A syntax reader must
construct `recipe_declaration` values and supply a separately sealed
`profile_catalog`.  Only `seal_source()` crosses the authority boundary.

`recipe.yml/1` uses explicit requirement mappings:

```yaml
requirements:
  build:
    - profile: "@toolchain"
    - package: pkg-config
  run:
    - package: libfoo
  lifecycle:
    post-install:
      - package: desktop-file-utils
```

There is no scalar shorthand and recipes cannot define or override profiles.

## Boundary

The library does not:

* scan collections or choose repository precedence;
* resolve package availability or dependency closure;
* download, copy, or verify source objects;
* execute build, check, or lifecycle programs;
* provide namespaces, Landlock, cgroups, or fakeroot behavior;
* create package images or archives;
* install packages or read installed state; or
* import Pkgfile/0 or historical package database records.

Those are separate architecture stages and migration programs.

See [DESIGN.md](DESIGN.md) for the authority model and [MIGRATION.md](MIGRATION.md)
for the compatibility boundary.

## Build

Shared and static libraries require separate Meson configurations.
`default_library=both` is rejected and `link_mode` must match the selected
library kind.

```sh
meson setup build-shared \
  -Ddefault_library=shared \
  -Dlink_mode=shared
meson compile -C build-shared
meson test -C build-shared --print-errorlogs

meson setup build-static \
  -Ddefault_library=static \
  -Dlink_mode=static
meson compile -C build-static
meson test -C build-static --print-errorlogs
```

Tests are enabled by default.  Manual pages are built when `scdoc` is available
or can be required with `-Dman_pages=enabled`.

## Optional planner adapter

`libpkgsource-plan` is enabled with `-Dplanner_adapter=enabled`.  It projects one
sealed source snapshot into a `libpkgplan` candidate package fact while retaining
the issuing source snapshot.

The adapter projects only exact run requirements, durable pre-remove and
post-remove program bytes, target architecture requirements, and package release
control.  It does not reinterpret syntax or profiles.  Build requirements,
check requirements, lifecycle requirements, selected build profiles, build
architecture requirements, install lifecycle programs, sources, and build
programs remain outside planner candidate control.

The supplied `libpkgplan` 0.2 API already provides the required value domains;
no downstream planner API change is required for version 1.

## ABI and license

The core library and optional planner adapter use SONAME 1.  Consumers must move
atomically from the removed version-0 API; no compatibility headers or binary
bridge are provided.

GPL-3.0-or-later.  See `COPYING` and `COPYRIGHT`.
