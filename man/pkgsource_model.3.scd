pkgsource_model(3)

# NAME

pkgsource_model - native package-source declaration value domains

# SYNOPSIS

*#include <libpkgsource/model.h>*

# DESCRIPTION

*package_reference*, *profile_reference*, and *architecture_reference* are
strict canonical ASCII identities.  Non-canonical spelling is rejected rather
than treated as an alias.

*requirement_scope* represents build, run, check, or one exact lifecycle action.
*requirement_subject* represents an exact package or profile reference.  A
package needed for build and runtime is represented by two declarations.  There
is no combined build-and-run scope.

*package_release* stores exact package coordinates and a domain-separated
semantic identity.  *package_metadata* stores summary, optional description and
homepage, and a normalized non-empty license set.

*source_input* represents an exact remote URL or safe relative local path,
explicit local name, and SHA-256 content requirement.  Local names are never
derived from locators.  MD5 is not accepted.

*program* retains exact non-empty POSIX-shell material and its raw SHA-256
content digest.  *lifecycle_program* binds one program to one exact lifecycle
action.  The library never executes either value.

*architecture_requirements* keeps independent normalized build and target
architecture sets.  An empty set means unrestricted.

*declaration_provenance* retains document, semantic path, line, and column.
Provenance is excluded from semantic identities.

# SEE ALSO

*libpkgsource*(3), *pkgsource_profile*(3), *pkgsource_recipe*(3)
