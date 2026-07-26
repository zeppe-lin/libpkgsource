<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# recipe.yml/1 input contract

`recipe.yml/1` is the initial native Zeppe-Lin recipe syntax.  It is an
input protocol, not the package-source authority.  The optional
`libpkgsource-yaml` adapter translates the YAML document into parser-neutral
declarations and passes them through the native sealer with an already sealed
profile catalog.  Other readers may implement the same contract.  Only the
returned `source_snapshot` may be consumed by later stages.

## Document shape

The exact version-one document is:

```yaml
format: zeppe-lin.recipe/1

package:
  name: example
  version: 1.2.3
  release: 1
  summary: Example package
  description: Optional longer description.
  homepage: https://example.invalid/
  licenses:
    - GPL-3.0-or-later

requirements:
  build:
    - profile: "@toolchain"
    - profile: "@meson"
    - package: pkg-config
  run:
    - package: libfoo
  check:
    - package: pkgcheck
  lifecycle:
    post-install:
      - package: desktop-file-utils

sources:
  - url: https://example.invalid/example-1.2.3.tar.xz
    name: example-1.2.3.tar.xz
    sha256: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
  - path: files/example.conf
    name: example.conf
    sha256: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef

build:
  language: posix-shell
  script: |
    meson setup build
    meson compile -C build
    meson install -C build --destdir "$PKG"

lifecycle:
  post-install:
    language: posix-shell
    script: |
      update-desktop-database

architectures:
  build:
    - x86_64
  target:
    - x86_64
```

The document is parsed as one strict YAML document.  Duplicate mapping keys,
unknown keys, non-scalar mapping keys, YAML directives, anchors, aliases, merge
keys, and custom or kind-incompatible tags are errors.  Standard string,
integer, sequence, and mapping tags are accepted only where their node kind
matches the schema.  Scalar shorthand for requirement subjects is not part of
version one.  `{package: NAME}` and
`{profile: "@NAME"}` are distinct syntax forms because their normalized
subjects are distinct authority domains.

## Package

`format` is required and must be exactly `zeppe-lin.recipe/1`.

`package.name` is a canonical exact package reference.  `package.version` is a
non-empty line-safe string without `/`.  `package.release` is a canonical
decimal integer greater than zero, without a sign or leading zeroes.  `summary`
and a non-empty `licenses` sequence are required.
`description` and `homepage` are optional.  License strings are retained as
normalized exact values; version one does not infer them from source files.

## Requirements

`requirements` is required.  Omitted scope lists are empty.

`build`, `run`, and `check` contain requirement subjects.  `check` is already a
typed native scope even though build execution and test execution are outside
this implementation series.

`requirements.lifecycle` maps an exact lifecycle action to subjects.  Valid
actions are `pre-install`, `post-install`, `pre-remove`, and `post-remove`.
Every lifecycle action that declares requirements must also declare a program
under `lifecycle`.

A package subject is an exact canonical package name.  It is not a capability,
provider expression, or version predicate.  A profile subject names an
external sealed profile value.  Recipes cannot define or override profiles.
The sealer rejects unknown profiles and retains the complete profile closure and
all expansion provenance in the source snapshot.

There is no `build-and-run` subject list.  A requirement needed in both scopes
is declared once under `build` and once under `run`.

## Sources

`sources` is required and may be empty.  Each entry has exactly one of `url` or
`path`, plus required `name` and `sha256` fields.

`url` is a remote locator.  `path` is a safe relative local source path without
empty, `.` or `..` components.  `name` is the exact local source identity used
by later fetch and build stages; it is never derived from the URL or path.
`sha256` is exactly 64 lowercase hexadecimal digits.  MD5 is not accepted.
Duplicate `name` values are errors.

The source model declares inputs and their required content identities.  It
does not download, copy, or verify them.

## Programs

`build` is required.  Version one accepts only `language: posix-shell` and an
exact non-empty YAML string in `script`.  The normalized model retains the
program bytes and their SHA-256 digest but never executes them.

`lifecycle` is optional and maps lifecycle actions to the same program shape.
Duplicate actions are errors.  Installation and removal programs remain
separate values; requirements are bound to the exact action rather than to a
generic lifecycle scope.

## Architectures

`architectures` is optional.  `build` and `target` are independent sequences of
canonical architecture identities.  Missing or empty sequences mean
unrestricted.  Non-empty sequences are sorted and duplicates are rejected.

Build architecture requirements constrain the future build environment.  Target
architecture requirements describe the package result.  Neither is a historical
`.32bit` compatibility marker.

## Sealing and identities

The sealer normalizes unordered sets, expands profiles deterministically,
rejects cycles and duplicates, binds lifecycle requirements, and computes
versioned domain-separated semantic identities.

Declaration locations are retained for diagnostics but excluded from semantic
identity.  Equivalent declarations in a different YAML order or at different
line numbers therefore seal to the same recipe identity.  Changes to package
release, metadata, exact requirements, profile identities or expansion paths,
source declarations, program bytes, lifecycle programs, or architecture
requirements change the recipe identity.

## YAML adapter API

`libpkgsource-yaml` parses raw document bytes and never opens paths.
`parse_recipe_yaml_v1()` returns a parser-neutral `parsed_recipe_document`;
`seal_recipe_yaml_v1()` additionally invokes the native sealer and returns the
authoritative `source_snapshot`.  Syntax failures throw structured
`yaml_error` values carrying a stable code, document, schema path, and one-based
line and column.  Invalid declaration values are reported at their syntax
location.  Failures from the native source sealer remain `pkgsource::error`
values.

The adapter does not scan collections, discover `profiles.yml`, download source
objects, execute programs, or resolve requirements.
