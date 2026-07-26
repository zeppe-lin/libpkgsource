pkgsource_plan_adapter(3)

# NAME

pkgsource_plan_adapter - project sealed native source truth into libpkgplan

# SYNOPSIS

*#include <libpkgsource-plan/adapter.h>*

*pkgsource::plan_adapter::project_candidate(source)*

# DESCRIPTION

*libpkgsource-plan* projects one sealed *source_snapshot* into a
*pkgplan::candidate_package_fact* and retains the source snapshot beside the
planner fact.

The projection contains exact run requirements, pre-remove and post-remove
program bytes, normalized target-architecture requirements, package release
coordinates, and source-issued package release identity.

Build requirements, check requirements, lifecycle requirements, selected build
profiles, build architecture requirements, installation lifecycle programs,
source inputs, and the build program are not planner candidate control.

# IDENTITIES

The source package-release SHA-256 identity is converted without reinterpretation
to the corresponding planner identity domain.  Candidate-control identity is
computed from planner-normalized control using the domain
*libpkgsource-plan/candidate-control/v2*.

The complete source snapshot identity remains separate provenance.

# ERRORS

Projection failures throw *pkgsource::plan_adapter::projection_error*.
*projection_error_code* distinguishes identity construction from planner fact
rejection.

# SEE ALSO

*libpkgsource*(3), *pkgsource_snapshot*(3), *libpkgplan*(3)
