<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Migration boundary

`libpkgsource 3.0.0` is an authority reset, not an in-place compatibility
upgrade.

## Repository split

The former optional in-tree adapters now have independent release boundaries:

- use `libpkgsource-yaml >= 1.0.0` for YAML parsing;
- use `libpkgsource-plan >= 1.0.0` for planner projection;
- use `libpkgsource-codec = 3.0.0` for durable owner records.

The core build has no adapter options, adapter dependencies, adapter headers, or
adapter manuals.

## Source declarations

Remove `source_syntax` and all recipe-format arguments from calls to
`seal_source()`:

```cpp
seal_source(origin, declaration, profiles)
```

Input grammar remains known to the parser or acquisition layer. It is not
source semantic authority.

The core no longer exposes `recipe_identity`. `source_snapshot_identity` binds
the complete normalized current source model directly.

## YAML transition

The standalone YAML frontend publishes one `zeppe-lin.recipe/1` protocol that
includes the optional check program. The experimental recipe/1 versus recipe/2
split is not retained. There are no versioned C++ parser entry points and no
parse-and-seal convenience functions.

## Identity transition

Source snapshots must be resealed under the
`libpkgsource/source-snapshot/v1` domain. Identities from the earlier development
line are not translated or aliased. No package-system evidence store had yet
admitted those identities, so carrying a compatibility decoder would create
legacy rather than preserve deployed authority.

Package-release and profile identities retain their established version-one
contracts.

## Durable record transition

The `libpkgsource 2.1.0` in-core codec API and its records are not accepted by
3.0. Re-encode freshly sealed 3.0 values through `libpkgsource-codec.so.1`.
There is no decoder for experimental source-record generations.

A missing native record remains missing authority. The decoder does not reopen
YAML, search collections, or infer replacement semantics from ambient files.

## Historical package sources

Pkgfile evaluation, CRUX metadata comments, sidecars, MD5 declarations,
`.32bit`, combined build-and-run scope, and historical package-database import
remain outside these libraries. Any importer must make ambiguity explicit and
produce complete native declarations for ordinary sealing.
