pkgsource_model(3)

# NAME

pkgsource_model - native package-source declaration value model

# SYNOPSIS

*#include <libpkgsource/model.h>*

# DESCRIPTION

Package, profile, and architecture references are strict canonical identities.
Non-canonical spelling is rejected rather than treated as an alias.

*requirement_scope* represents build, run, check, or one lifecycle action.
*requirement_subject* represents an exact package or profile reference.  There
is no combined build-and-run scope.

*package_release*, *package_metadata*, *source_input*, *program*,
*lifecycle_program*, and *architecture_requirements* provide the native values
needed by later source sealing.

*declaration_provenance* retains the exact document, semantic path, line, and
column that issued a declaration.  Provenance is diagnostic and does not become
semantic identity.

# SEE ALSO

*libpkgsource*(3)
