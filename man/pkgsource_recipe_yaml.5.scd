pkgsource_recipe_yaml(5)

# NAME

pkgsource_recipe_yaml - native recipe.yml/1 input syntax

# DESCRIPTION

A *recipe.yml/1* document declares one native package release.  The root mapping
uses *format: zeppe-lin.recipe/1* and contains package metadata, typed
requirements, source declarations, a build program, optional lifecycle
programs, and optional build and target architecture constraints.

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
lowercase SHA-256 digest.  Version one accepts only exact *posix-shell* program
bytes.  The parser does not fetch inputs or execute programs.

# AUTHORITY

The parser produces a *recipe_declaration*.  It becomes authority only after
*seal_source()* validates it with an already sealed profile catalog and returns
a *source_snapshot*.

Document spelling, mapping order, and declaration positions do not become
semantic identity.  Exact package semantics, profile identities, source
identities, programs, and architecture constraints do.

# SEE ALSO

*pkgsource_yaml*(3), *pkgsource_recipe*(3), *pkgsource_profiles_yaml*(5)
