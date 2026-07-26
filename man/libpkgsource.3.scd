libpkgsource(3)

# NAME

libpkgsource - immutable package-source inspection library

# SYNOPSIS

*#include <libpkgsource/libpkgsource.h>*

# DESCRIPTION

*libpkgsource* inspects one package-source directory and returns a normalized,
immutable *source_snapshot*(3). Version 0.2.0 implements only the *pkgfile/0*
backend.

The library owns source-format probing, package identity, descriptive metadata,
legacy dependency declarations, source declarations and checksums, recipe and
lifecycle descriptors, README resources, strip exclusions, footprint
declaration, architecture marker, and a private source snapshot.

It does not scan collections, select precedence, resolve dependencies, download
or verify distfiles, execute recipes or lifecycle programs, construct package
images, inspect archives, install packages, or access installed state.

# BUILD

Shared and static libraries require separate Meson configurations. The project
rejects *default_library=both*. The corresponding *link_mode* option must match
*default_library*.

# ERRORS

Contract failures throw *pkgsource::error*. Callers should inspect its typed
*error_code* rather than parse diagnostic text.

# SECURITY

Pkgfiles are executable shell. The private worker uses an explicit environment,
working directory, umask, and optional uid/gid, and validates a NUL-framed
record. It is not a syscall or network sandbox.

# SEE ALSO

*pkgsource_model*(3), *pkgsource_backend*(3),
*pkgsource_pkgfile_backend*(3), *pkgsource_snapshot*(3),
*pkgsource_plan_adapter*(3)
