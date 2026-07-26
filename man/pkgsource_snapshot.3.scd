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
different document location seal to the same recipe and snapshot identities.

Version 1 records *source_syntax::recipe_yaml_v1*.  The YAML document itself is
not authority and the library does not parse it.

# SEE ALSO

*libpkgsource*(3), *pkgsource_recipe*(3), *pkgsource_plan_adapter*(3)
