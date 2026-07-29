pkgsource_snapshot(3)

# NAME

pkgsource_snapshot - immutable sealed source authority

# SYNOPSIS

*#include <libpkgsource/snapshot.h>*

# DESCRIPTION

*seal_source()* seals one *recipe_declaration* with a *profile_catalog* and
returns a *source_snapshot*.

The snapshot retains *source_origin* and *source_syntax* for diagnostics, the
complete *sealed_recipe*, and a distinct source-snapshot identity.  Origin and
syntax do not become package semantics.  Equivalent native declarations from a
different document location or syntax may seal to the same recipe and snapshot
identities.

*source_syntax::recipe_yaml_v1* records *recipe.yml/1*.  Version one does not
admit a check program and may retain check requirements as a reserved,
non-executable requirement domain.

*source_syntax::recipe_yaml_v2* records *recipe.yml/2*.  Version two admits one
optional exact check program.  Check requirements without that program are
rejected.  A check program with no additional check requirements is valid.

The YAML document itself is not authority.  The core library does not parse it;
the optional *libpkgsource-yaml* syntax adapter produces declarations for the
existing sealer.

# SEE ALSO

*libpkgsource*(3), *pkgsource_recipe*(3), *pkgsource_plan_adapter*(3),
*pkgsource_yaml*(3), *pkgsource_recipe_yaml*(5)
