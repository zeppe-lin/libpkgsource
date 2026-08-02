pkgsource_profile(3)

# NAME

pkgsource_profile - sealed authoritative requirement profiles

# SYNOPSIS

*#include <libpkgsource/profile.h>*

# DESCRIPTION

*profile_declaration* is parser-neutral input.  It contains one canonical
profile name, its declaration provenance, and direct exact package or profile
members.

*profile_catalog::seal()* sorts definitions and members deterministically and
rejects empty profiles, duplicate definitions, duplicate direct members,
unknown nested references, and cycles.  The returned catalog contains immutable
*sealed_profile* values.

A sealed profile retains its direct members, exact transitive expansion paths,
all declaration provenance on those paths, and a versioned semantic identity.
Nested profile identity contributes to parent identity.

*sealed_requirement_set::seal()* expands recipe requirement declarations into
exact package requirements.  It retains every direct or profile expansion
origin, selected build-profile roots, and the complete transitive profile
closure used by the recipe.

Profiles are authority values.  They are not parser aliases and recipes cannot
redefine them.

# SEE ALSO

*libpkgsource*(3), *pkgsource_model*(3), *pkgsource_recipe*(3)
