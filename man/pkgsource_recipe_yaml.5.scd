pkgsource_recipe_yaml(5)

# NAME

pkgsource_recipe_yaml - native recipe.yml/1 and recipe.yml/2 input syntax

# DESCRIPTION

A native recipe YAML document declares one package release.  The root mapping
uses either *format: zeppe-lin.recipe/1* or
*format: zeppe-lin.recipe/2* and contains package metadata, typed requirements,
source declarations, a build program, optional lifecycle programs, and optional
build and target architecture constraints.

Requirement subjects are explicit mappings:

```
requirements:
  build:
    - profile: "@toolchain"
    - package: pkg-config
  run:
    - package: libfoo
```

Build, run, check, and action-bound lifecycle scopes remain distinct.  There is
no combined build-and-run scope and no scalar subject shorthand.

Each source has exactly one *url* or *path*, plus an explicit local *name* and a
lowercase SHA-256 digest.  Programs use exact *language: posix-shell* and
*script* fields.  The parser does not fetch inputs or execute programs.

# VERSION ONE

*recipe.yml/1* is the published original native protocol.  It does not accept a
root check program.  Check-scoped requirements remain valid reserved metadata
and do not by themselves provide executable check authority.

# VERSION TWO

*recipe.yml/2* adds one optional root *check* program with the same shape as the
build and lifecycle programs:

```
check:
  language: posix-shell
  script: |
    meson test -C build
```

Version-two check requirements require this program.  A check program with no
additional check requirements is valid.  Program bytes are retained exactly and
contribute to semantic identity, but execution policy and result evidence remain
outside *libpkgsource*.

# AUTHORITY

The parser produces a *recipe_declaration*.  It becomes authority only after
*seal_source()* validates it with an already sealed profile catalog and returns
a *source_snapshot*.

Document spelling, mapping order, syntax version, and declaration positions do
not independently become semantic identity.  Exact package semantics, profile
identities, source identities, programs, and architecture constraints do.

# SEE ALSO

*pkgsource_yaml*(3), *pkgsource_recipe*(3), *pkgsource_profiles_yaml*(5)
