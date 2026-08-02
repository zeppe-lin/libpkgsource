% PKGSOURCE_RECIPE(3) libpkgsource 3.0.0 | libpkgsource

# NAME

pkgsource_recipe - seal normalized native recipe authority

# SYNOPSIS

```cpp
#include <libpkgsource/recipe.h>
```

# DESCRIPTION

`pkgsource::recipe_declaration` is parser-neutral input. It contains package
release, metadata, source inputs, one exact build program, an optional exact
check program, requirement declarations, lifecycle programs, architecture
requirements, and declaration provenance.

`pkgsource::seal_recipe()` performs deterministic normalization. It sorts source
and lifecycle domains, rejects duplicate source local names and lifecycle
actions, expands package and profile requirements through one sealed profile
catalog, rejects lifecycle requirements without a program for the same action,
and rejects check requirements without a check program.

A check program with no additional check requirements is valid.

The returned `pkgsource::sealed_recipe` retains normalized semantic fields,
selected build profiles, the exact profile closure, all resolved requirement
origins, and diagnostic provenance. It has no independent recipe identity; the
complete normalized recipe contributes directly to
`pkgsource::source_snapshot_identity`.

# BOUNDARY

Recipe declarations and sealed recipes contain no parser generation or input
syntax tag. Parsing, acquisition, resolution, fetching, execution, and planning
are outside the core.

# SEE ALSO

`libpkgsource(3)`, `pkgsource_profile(3)`, `pkgsource_snapshot(3)`
