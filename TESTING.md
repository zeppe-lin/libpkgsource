<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Testing

The suite is offline and model-driven.  When the optional YAML adapter is
enabled it parses in-memory documents through libyaml; it does not open files,
inspect package collections, execute recipe programs, or access installed
state.

Coverage includes:

* canonical package, profile, and architecture identity rejection;
* package-release and raw program SHA-256 identity vectors;
* independent build, run, check, and lifecycle requirement scopes;
* exact package and profile requirement subjects;
* action binding for lifecycle requirements;
* deterministic profile sealing independent of declaration insertion order;
* nested profile expansion and retained edge provenance;
* unknown-profile, duplicate-definition, duplicate-member, and cycle rejection;
* selected build-profile roots and complete transitive profile closure;
* exact requirement origin aggregation across profile paths;
* source input normalization, safe local paths, SHA-256-only content identity,
  and duplicate local-name rejection;
* separate build and target architecture requirements;
* lifecycle program uniqueness and lifecycle-requirement/program closure;
* recipe identity stability across non-semantic source order and provenance;
* recipe identity change sensitivity to program and profile semantics;
* source snapshot identity and syntax-provenance separation;
* independent public-header compilation;
* native source-to-planner candidate projection;
* exclusion of build, check, and lifecycle requirements from planner runtime
  control;
* exclusion of installation lifecycle programs from durable removal control;
* target architecture projection; and
* independent `libpkgsource-plan` public-header compilation;
* strict one-document YAML parsing and structured syntax provenance;
* duplicate/unknown key, alias, anchor, merge-key, directive, tag, and type
  rejection;
* exact `profiles.yml/1` declarations and deterministic profile sealing;
* exact `recipe.yml/1` declarations and identity-equivalent reordered input;
* syntax-versus-semantic error-domain separation;
* independent `libpkgsource-yaml` public-header compilation; and
* generated `libpkgsource-yaml.pc` dependency metadata.

Run the core shared configuration:

```sh
meson setup build-shared \
  -Ddefault_library=shared \
  -Dlink_mode=shared
meson compile -C build-shared
meson test -C build-shared --print-errorlogs
```

Run the optional adapter against an installed `libpkgplan`:

```sh
PKG_CONFIG_PATH=/path/to/dependencies/lib/pkgconfig \
meson setup build-plan \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
  -Dplanner_adapter=enabled
meson compile -C build-plan
meson test -C build-plan --print-errorlogs
```

Repeat with `default_library=static` and `link_mode=static` for the supported
static closure.  `default_library=both` is deliberately invalid.

The release metadata test binds project version, SONAMEs, dependency floor,
history heading, README version contract, and installed public headers.  It is
part of the normal suite.


Run the YAML adapter:

```sh
meson setup build-yaml \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
  -Dyaml_adapter=enabled
meson compile -C build-yaml
meson test -C build-yaml --print-errorlogs
```
