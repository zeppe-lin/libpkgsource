pkgsource_snapshot(3)

# NAME

pkgsource_snapshot - immutable sealed source authority

# SYNOPSIS

*#include <libpkgsource/snapshot.h>*

*pkgsource::source_snapshot pkgsource::seal_source(*
*    pkgsource::source_origin origin,*
*    pkgsource::recipe_declaration declaration,*
*    const pkgsource::profile_catalog& profiles);*

# DESCRIPTION

*seal_source()* passes the parser-neutral declaration through *seal_recipe()*
and returns one immutable *source_snapshot*.

The snapshot retains diagnostic *source_origin*, the complete *sealed_recipe*,
and a distinct *source_snapshot_identity*.  Origin and declaration provenance
do not contribute to semantic identity.

The source identity is SHA-256 over the complete normalized current source
model under the domain *libpkgsource/source-snapshot/v1*.  It includes package
release and metadata, source inputs, build and optional check programs, resolved
requirements and profile expansion paths, retained profile identities,
lifecycle programs, and build/target architecture constraints.

There is no source-syntax value and no recipe-format generation in core
authority.  Equivalent declarations produced by different frontends or at
different document locations seal to the same source identity.

# SEE ALSO

*libpkgsource*(3), *pkgsource_recipe*(3), *pkgsource_codec*(3)
