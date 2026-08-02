libpkgsource(3)

# NAME

libpkgsource - native sealed package-source authority

# SYNOPSIS

*#include <libpkgsource/libpkgsource.h>*

# DESCRIPTION

*libpkgsource* validates parser-neutral native package declarations, seals
requirement profiles, expands exact requirements, and returns immutable
*source_snapshot* values.

Input syntax is provenance, not authority.  The optional
The separate *libpkgsource-yaml* library parses native YAML documents into parser-neutral declarations.  A declaration
becomes authority only when the existing profile or source sealer accepts it.

The normalized authority distinguishes package release, package metadata,
source inputs, build program, optional check program, build requirements,
runtime requirements, check requirements, lifecycle programs, action-bound
lifecycle requirements, build and target architecture requirements, selected
build profiles, and the exact profile closure used during expansion.

Version one retains check requirements as a reserved domain without check
program authority.  Version two may bind them to one exact optional check
program.  The library retains program bytes but does not execute them.

There is no Pkgfile backend, historical CRUX compatibility model, MD5 source
identity, or combined build-and-run requirement scope.

# BOUNDARY

The library does not scan collections, resolve package availability, download
or verify sources, execute programs, create package images, install packages,
read installed state, provide sandboxing, or perform migration.

# BUILD

Shared and static libraries require separate Meson configurations.  The project
rejects *default_library=both*.  *link_mode* must select the same library kind.

# ERRORS

Contract failures throw *pkgsource::error*.  Branch on *error_code*, not on
diagnostic text.

# SEE ALSO

*pkgsource_model*(3), *pkgsource_profile*(3), *pkgsource_recipe*(3),
*pkgsource_snapshot*(3)
