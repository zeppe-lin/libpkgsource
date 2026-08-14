% PKGSOURCE_SNAPSHOT(3) libpkgsource 4.0.0 | libpkgsource

# NAME

pkgsource_snapshot - seal immutable package-source authority

# SYNOPSIS

```cpp
#include <libpkgsource/snapshot.h>

pkgsource::source_snapshot pkgsource::seal_source(
    pkgsource::source_origin origin,
    pkgsource::recipe_declaration declaration,
    const pkgsource::profile_catalog& profiles);
```

# DESCRIPTION

`pkgsource::seal_source()` passes the parser-neutral declaration through
`pkgsource::seal_recipe()` and returns one immutable
`pkgsource::source_snapshot`.

The snapshot retains diagnostic `pkgsource::source_origin`, the complete
`pkgsource::sealed_recipe`, and a distinct
`pkgsource::source_snapshot_identity`. The public reconstruction constructor
recomputes that identity from the supplied recipe and rejects a mismatched
claim. Origin and declaration provenance do not contribute to semantic
identity.

The source identity is SHA-256 over the complete normalized source model under
the domain `libpkgsource/source-snapshot/v1`. It includes package release and
metadata, source inputs, build and optional check programs, resolved
requirements and profile expansion paths, retained profile identities,
lifecycle programs, and build and target architecture constraints.

There is no source-syntax value and no recipe-format generation in core
authority. Equivalent declarations produced by different frontends or at
different document locations seal to the same source identity.

# SEE ALSO

`libpkgsource(3)`, `pkgsource_recipe(3)`, `pkgsource_codec(3)`
