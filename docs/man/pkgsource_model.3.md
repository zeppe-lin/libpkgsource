% PKGSOURCE_MODEL(3) libpkgsource 3.0.1 | libpkgsource

# NAME

pkgsource_model - describe parser-neutral package-source value domains

# SYNOPSIS

```cpp
#include <libpkgsource/model.h>
```

# DESCRIPTION

`pkgsource::package_reference`, `pkgsource::profile_reference`, and
`pkgsource::architecture_reference` are strict canonical ASCII identities.
Non-canonical spelling is rejected rather than treated as an alias.

`pkgsource::requirement_scope` represents build, run, check, or one exact
lifecycle action. `pkgsource::requirement_subject` represents an exact package
or profile reference. A package needed for build and runtime is represented by
two declarations; there is no combined build-and-run scope.

`pkgsource::package_release` stores exact package coordinates and a semantic
identity. `pkgsource::package_metadata` stores summary, optional description and
homepage, and a normalized non-empty license set.

`pkgsource::source_input` represents an exact remote URL or safe relative local
path, an explicit local name, and a SHA-256 content requirement. Local names are
never derived from locators. MD5 is not accepted.

`pkgsource::program` retains exact non-empty POSIX-shell material and its raw
SHA-256 content digest. `pkgsource::lifecycle_program` binds one program to one
exact lifecycle action. Unsupported C++ enum values are rejected at these value
boundaries rather than canonicalized as a supported protocol value. The library
never executes either value.

`pkgsource::architecture_requirements` keeps independent normalized build and
target architecture sets. An empty set means unrestricted.

`pkgsource::declaration_provenance` retains document, semantic path, line, and
column. Provenance is diagnostic and is excluded from semantic identities.

# SEE ALSO

`libpkgsource(3)`, `pkgsource_profile(3)`, `pkgsource_recipe(3)`
