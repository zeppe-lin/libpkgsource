pkgsource_recipe(3)

# NAME

pkgsource_recipe - normalized native recipe authority

# SYNOPSIS

*#include <libpkgsource/recipe.h>*

# DESCRIPTION

*recipe_declaration* is parser-neutral input.  It contains package release,
metadata, source inputs, a build program, an optional check program, requirement
declarations, lifecycle programs, architecture requirements, and top-level
provenance.

The original constructor without a check-program argument remains available and
constructs a declaration with no check program.  *check_program()* returns the
optional exact program value.

*seal_recipe()* resolves all profile subjects through a sealed
*profile_catalog*, sorts source and lifecycle domains, rejects duplicate source
names and lifecycle programs, and rejects lifecycle requirements whose action
has no lifecycle program.

The returned *sealed_recipe* distinguishes build, run, check, and action-bound
lifecycle requirements.  It retains the optional check program, selected
build-profile roots, and the exact profile closure used during sealing.

Source-syntax closure is enforced by *seal_source()*.  *recipe.yml/1* cannot
carry a check program.  Under *recipe.yml/2*, check requirements require a check
program; a check program with no additional check requirements is valid.

The recipe identity is a domain-separated SHA-256 value over normalized semantic
content.  Declaration order and provenance do not change it.  Version-one
identity encoding remains unchanged when no check program is present.  A check
program selects the version-two recipe identity domain and contributes its
language and exact bytes.

# SEE ALSO

*libpkgsource*(3), *pkgsource_profile*(3), *pkgsource_snapshot*(3),
*pkgsource_recipe_yaml*(5)
