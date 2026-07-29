<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# recipe.yml/2 input contract

`recipe.yml/2` extends the native Zeppe-Lin recipe syntax with one optional,
exact check program.  It remains an input protocol rather than package-source
authority.  The optional `libpkgsource-yaml` adapter translates the document
into parser-neutral declarations, and the native source sealer produces the
only authoritative `source_snapshot`.

Version two preserves every version-one field and meaning.  Its format marker is
`zeppe-lin.recipe/2`, and the only new root field is `check`.

## Document shape

```yaml
format: zeppe-lin.recipe/2

package:
  name: example
  version: 1.2.3
  release: 1
  summary: Example package
  licenses:
    - GPL-3.0-or-later

requirements:
  build:
    - profile: "@toolchain"
  run:
    - package: libfoo
  check:
    - package: pkgcheck

sources:
  - url: https://example.invalid/example-1.2.3.tar.xz
    name: example-1.2.3.tar.xz
    sha256: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef

build:
  language: posix-shell
  script: |
    meson setup build
    meson compile -C build
    meson install -C build --destdir "$PKG"

check:
  language: posix-shell
  script: |
    meson test -C build
```

Package metadata, requirements, sources, build programs, lifecycle programs,
and architecture constraints have the exact version-one contracts described in
[RECIPE-YAML-1.md](RECIPE-YAML-1.md).

## Check program

`check` is optional.  When present, it has the same strict program shape as
`build` and lifecycle programs:

```yaml
check:
  language: posix-shell
  script: |
    meson test -C build
```

Version two accepts only `language: posix-shell`.  `script` is an exact non-empty
YAML string.  The normalized model retains the exact bytes and their SHA-256
digest.  Neither the parser nor the semantic core executes them.

A version-two recipe that declares `requirements.check` must also declare a
`check` program.  A check program with no additional check requirements is
valid.  The requirements name package inputs needed by a later check execution
stage; they are not an instruction to execute the program.

This closure rule belongs to the native source sealer.  A syntax reader cannot
bypass it by constructing a parser-neutral declaration directly.

## Version-one compatibility

`recipe.yml/1` remains unchanged.  It does not accept a root `check` program and
may retain check-scoped requirements as a reserved, non-executable requirement
domain.  Existing version-one declarations and their recipe and source-snapshot
identities remain stable.

A version-two document without a check program and without check requirements
has no additional package semantics.  Syntax version and document origin remain
diagnostic provenance rather than semantic identity.

## Identities

When a check program is present, its language and exact material contribute to
the normalized recipe identity and therefore to the source-snapshot identity.
Changing the check bytes changes both identities.  Requirement order and source
provenance remain non-semantic as in version one.

The check program is source authority only.  It does not define execution
paths, environment variables, success policy, sandbox policy, check-result
evidence, transaction binding, or scheduling.  Those belong to later
architectural boundaries.

## YAML adapter API

`parse_recipe_yaml_v2()` parses one strict version-two document without sealing
it.  `seal_recipe_yaml_v2()` additionally invokes the native source sealer with
an already sealed profile catalog.

The adapter rejects duplicate or unknown keys, multiple documents, directives,
anchors, aliases, merge keys, unsupported tags, scalar requirement shorthand,
and schema or type drift.  Syntax failures remain structured `yaml_error`
values.  Semantic closure failures remain `pkgsource::error` values.
