pkgsource_recipe(3)

# NAME

pkgsource_recipe - normalized native recipe authority

# SYNOPSIS

*#include <libpkgsource/recipe.h>*

# DESCRIPTION

*recipe_declaration* is parser-neutral input.  It contains package release,
metadata, source inputs, one exact build program, an optional exact check
program, requirement declarations, lifecycle programs, architecture
requirements, and declaration provenance.

The constructor without a check-program argument creates a declaration with no
check program.  *check_program()* returns the optional value.

*seal_recipe()*:

- sorts source and lifecycle domains deterministically;
- rejects duplicate source local names and lifecycle actions;
- expands package and profile requirements through one sealed profile catalog;
- rejects lifecycle requirements without a program for the same action; and
- rejects check requirements without a check program.

A check program with no additional check requirements is valid.

The returned *sealed_recipe* retains normalized semantic fields, selected build
profiles, the exact profile closure, all resolved requirement origins, and
diagnostic provenance.  It has no independent recipe identity; the complete
normalized recipe contributes directly to *source_snapshot_identity*.

# BOUNDARY

Recipe declarations and sealed recipes contain no parser generation or input
syntax tag.  Parsing, collection acquisition, resolution, source fetching,
execution, and planning are outside the core.

# SEE ALSO

*libpkgsource*(3), *pkgsource_profile*(3), *pkgsource_snapshot*(3)
