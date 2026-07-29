<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# libpkgsource

`libpkgsource` is the native Zeppe-Lin C++17 package-source authority.

It validates parser-neutral declarations, seals authoritative requirement
profiles, expands exact package requirements, and returns immutable normalized
recipe and source-snapshot values.  Input syntax is provenance.  The sealed
`source_snapshot` is authority.

The native reset is intentionally incompatible with the historical
implementation.  It contains no Pkgfile backend, no CRUX package-source
compatibility model, no MD5 source declarations, no sidecar semantics, and no
combined `build_and_run` requirement scope.  Historical conversion belongs to a
separate migration tool.

Version 2 adds explicit check-program authority.  Because the public C++ value
layouts change, the core, YAML adapter, and planner adapter all begin SONAME 2.

## Native model

The normalized snapshot distinguishes:

* package release and package metadata;
* remote and local source input declarations with SHA-256 identities;
* exact build program material and optional check program material;
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

`check` is a native typed requirement scope.  `recipe.yml/1` retains that
scope as reserved non-executable metadata.  `recipe.yml/2` may bind it to one
exact optional check program.  Neither form adds test execution to the library.

## Input syntax

The native syntax contracts are [PROFILES-YAML-1.md](PROFILES-YAML-1.md),
[RECIPE-YAML-1.md](RECIPE-YAML-1.md), and
[RECIPE-YAML-2.md](RECIPE-YAML-2.md).  The optional `libpkgsource-yaml` adapter
parses raw document bytes into parser-neutral declarations.  It never opens
paths or scans collections.  Only the existing profile and source sealers cross
the authority boundary.

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
Version two adds one optional top-level `check` program with the same exact
`language` and `script` shape as build and lifecycle programs.  Check
requirements in version two require that program; the program itself needs no
additional requirements.

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

## Optional YAML adapter

`libpkgsource-yaml` is enabled with `-Dyaml_adapter=enabled` and requires
libyaml 0.2.5 or later.  It provides strict `profiles.yml/1`, `recipe.yml/1`,
and `recipe.yml/2` parsers, structured syntax diagnostics,
parser-neutral declarations, and convenience functions that invoke the native
sealers.  The adapter rejects
duplicate keys, unknown fields, multiple documents, anchors, aliases, merge
keys, unsupported tags, scalar requirement shorthand, and schema/type drift.

The adapter is syntax only.  It does not discover collections, combine profile
documents, choose precedence, resolve requirements, fetch sources, or execute
programs.

## Optional planner adapter

`libpkgsource-plan` is enabled with `-Dplanner_adapter=enabled`.  It projects one
sealed source snapshot into a `libpkgplan` candidate package fact while retaining
the issuing source snapshot.

The adapter projects only exact run requirements, durable pre-remove and
post-remove program bytes, target architecture requirements, and package release
control.  It does not reinterpret syntax or profiles.  Build requirements,
check requirements, lifecycle requirements, selected build profiles, build
architecture requirements, install lifecycle programs, sources, build
programs, and check programs remain outside planner candidate control.

The supplied `libpkgplan` 0.2 API already provides the required value domains;
no downstream planner API change is required for this projection.

## ABI and license

The core library, optional planner adapter, and optional YAML adapter use
SONAME 2.  Consumers must rebuild atomically; public C++ recipe and snapshot
value layouts changed when check-program authority was added.  No binary bridge
is provided.

GPL-3.0-or-later.  See `COPYING` and `COPYRIGHT`.
