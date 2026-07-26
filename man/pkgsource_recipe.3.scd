pkgsource_recipe(3)

# NAME

pkgsource_recipe - normalized native recipe authority

# SYNOPSIS

*#include <libpkgsource/recipe.h>*

# DESCRIPTION

*recipe_declaration* is parser-neutral input.  It contains package release,
metadata, source inputs, build program, requirement declarations, lifecycle
programs, architecture requirements, and top-level provenance.

*seal_recipe()* resolves all profile subjects through a sealed
*profile_catalog*, sorts source and lifecycle domains, rejects duplicate source
names and lifecycle programs, and rejects lifecycle requirements whose action
has no lifecycle program.

The returned *sealed_recipe* distinguishes build, run, check, and action-bound
lifecycle requirements.  It retains selected build-profile roots and the exact
profile closure used during sealing.

The recipe identity is a domain-separated SHA-256 value over normalized semantic
content.  Declaration order and provenance do not change it.  Changes to package
release, metadata, source declarations, exact requirements, profile semantics,
program bytes, lifecycle programs, or architecture requirements do.

# SEE ALSO

*libpkgsource*(3), *pkgsource_profile*(3), *pkgsource_snapshot*(3)
