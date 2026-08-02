% LIBPKGSOURCE(3) libpkgsource 3.0.0 | libpkgsource

# NAME

libpkgsource - seal parser-neutral package-source authority

# SYNOPSIS

```cpp
#include <libpkgsource/libpkgsource.h>
```

# DESCRIPTION

`libpkgsource` validates parser-neutral declarations, seals requirement
profiles, expands exact package requirements, and returns immutable
`pkgsource::source_snapshot` values.

Input syntax is not authority. A profile declaration becomes authority only
through `pkgsource::profile_catalog::seal()`. A recipe declaration becomes
authority only through `pkgsource::seal_source()` with one explicit sealed
profile catalog.

The normalized source authority distinguishes package release, metadata, source
inputs, exact build and optional check programs, build, run, check, and
action-bound lifecycle requirements, lifecycle programs, independent build and
target architecture constraints, selected build profiles, and the complete
retained profile closure.

Check requirements require a check program. Lifecycle requirements require a
program for the same action. The library retains exact program bytes but never
executes them.

# IDENTITIES

Package releases and profiles use domain-separated SHA-256 identities. The
complete source model uses the `libpkgsource/source-snapshot/v1` domain.
Diagnostic source origin and declaration provenance do not contribute to
semantic identity.

# BOUNDARY

The core does not parse YAML, open paths, discover collections, choose
precedence, resolve packages, fetch objects, execute programs, create images,
install packages, publish state, or project planner facts.

The separate `libpkgsource-codec` library owns canonical durable records. The
separate `libpkgsource-yaml` and `libpkgsource-plan` repositories own syntax
parsing and planner projection.

# ERRORS

Contract failures throw `pkgsource::error`. Consumers should branch on
`pkgsource::error_code`, not diagnostic text.

# ABI

The core ABI is `libpkgsource.so.3`.

# SEE ALSO

`pkgsource_model(3)`, `pkgsource_profile(3)`, `pkgsource_recipe(3)`,
`pkgsource_snapshot(3)`, `pkgsource_codec(3)`
