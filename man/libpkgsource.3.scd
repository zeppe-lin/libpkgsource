libpkgsource(3)

# NAME

libpkgsource - native sealed package-source model library

# SYNOPSIS

*#include <libpkgsource/libpkgsource.h>*

# DESCRIPTION

*libpkgsource* defines the native Zeppe-Lin package-source value domains.  Input
syntax is not authority.  Parser-neutral declarations are validated and later
sealed into immutable profile and recipe values.

Version 1 removes the historical Pkgfile backend and all CRUX compatibility
semantics.  The library does not execute build or lifecycle programs.

# BUILD

Shared and static libraries require separate Meson configurations.  The project
rejects *default_library=both*.  *link_mode* must select the same library kind.

# ERRORS

Contract failures throw *pkgsource::error*.  Branch on *error_code*, not on
diagnostic text.

# SEE ALSO

*pkgsource_model*(3)
