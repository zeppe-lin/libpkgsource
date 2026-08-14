% PKGSOURCE_PROFILE(3) libpkgsource 4.1.0 | libpkgsource

# NAME

pkgsource_profile - seal authoritative requirement profiles

# SYNOPSIS

```cpp
#include <libpkgsource/profile.h>
```

# DESCRIPTION

`pkgsource::profile_declaration` is parser-neutral input. It contains one
canonical profile name, declaration provenance, and direct exact package or
profile members.

`pkgsource::profile_catalog::seal()` sorts definitions and members
deterministically and rejects duplicate definitions, duplicate direct members,
unknown nested references, and cycles. The returned catalog contains immutable
`pkgsource::sealed_profile` values.

A sealed profile retains its direct members, exact transitive expansion paths,
all declaration provenance on those paths, and a semantic identity. Nested
profile identity contributes to parent identity. The public reconstruction
constructor authenticates canonical direct-member order, expansion continuity,
retained member provenance, and the claimed semantic identity.

`pkgsource::sealed_requirement_set::seal()` expands recipe requirement
declarations into exact package requirements. It retains every direct or
profile expansion origin, selected build-profile roots, and the complete
transitive profile closure used by the recipe.

Profiles are authority values. They are not parser aliases, and recipes cannot
redefine them.

# SEE ALSO

`libpkgsource(3)`, `pkgsource_model(3)`, `pkgsource_recipe(3)`
