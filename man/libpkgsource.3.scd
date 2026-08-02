libpkgsource(3)

# NAME

libpkgsource - native sealed package-source authority

# SYNOPSIS

*#include <libpkgsource/libpkgsource.h>*

# DESCRIPTION

*libpkgsource* validates parser-neutral native declarations, seals requirement
profiles, expands exact package requirements, and returns immutable
*source_snapshot* values.

Input syntax is not authority.  Syntax frontends construct declarations.  A
profile declaration becomes authority only through *profile_catalog::seal()*.
A recipe declaration becomes authority only through *seal_source()* with one
explicit sealed profile catalog.

The normalized source authority distinguishes package release, metadata, source
inputs, exact build and optional check programs, build/run/check/action-bound
requirements, lifecycle programs, independent build and target architecture
constraints, selected build profiles, and the complete retained profile closure.

Check requirements require a check program.  Lifecycle requirements require a
program for the same action.  The library retains exact program bytes but never
executes them.

# IDENTITIES

Package releases and profiles use version-one domain-separated SHA-256
identities.  The complete current source model uses
*libpkgsource/source-snapshot/v1*.  Diagnostic source origin and declaration
provenance are excluded from semantic identity.

# BOUNDARY

The core has no YAML, collection, planner, execution, state, or storage
integration.  It does not open paths, discover collections, choose precedence,
resolve packages, fetch or verify source objects, execute programs, create
images, install packages, or import historical formats.

The separate *libpkgsource-codec* library owns canonical durable records for
sealed source authority.  The separate *libpkgsource-yaml* and
*libpkgsource-plan* repositories own syntax parsing and planner projection.

# BUILD

Shared and static libraries require separate Meson configurations.  The project
rejects *default_library=both*.  *link_mode* must select the same library kind.

# ERRORS

Contract failures throw *pkgsource::error*.  Branch on *error_code*, not on
diagnostic text.

# ABI

The core ABI is *libpkgsource.so.3*.

# SEE ALSO

*pkgsource_model*(3), *pkgsource_profile*(3), *pkgsource_recipe*(3),
*pkgsource_snapshot*(3), *pkgsource_codec*(3)
