pkgsource_plan_adapter(3)

# NAME

pkgsource_plan_adapter - project sealed package-source truth into libpkgplan

# SYNOPSIS

*#include <libpkgsource-plan/adapter.h>*

*pkgsource::plan_adapter::candidate_projection*++
*pkgsource::plan_adapter::project_candidate(* \
    *pkgsource::source_snapshot source*);

# DESCRIPTION

*libpkgsource-plan* is an optional composition library. It translates one
immutable *source_snapshot*(3) into one
*pkgplan::candidate_package_fact* while retaining ownership of the source
snapshot that issued the facts.

The projection contains:

. package name, version, and release;
. runtime dependency declarations whose source scope includes runtime use;
. exact pre-remove and post-remove lifecycle bytes;
. the package build-architecture profile fact;
. a source-issued package-release identity; and
. a source-issued normalized candidate-control identity.

Pre-install and post-install lifecycle programs are not removal control and are
not projected.

# IDENTITIES

Package-release and candidate-control identities use domain-separated,
versioned SHA-256 canonical records.

Candidate-control identity is computed after planner normalization. Unrelated
source bytes such as README content, recipe implementation, and source-input
material do not change that identity. Changes to runtime dependencies, removal
lifecycle bytes, or build architecture do change it.

The complete source snapshot fingerprint remains separate provenance. It is not
relabelled as a planner-domain identity.

# LIFETIME

*candidate_projection* owns the source snapshot and the planner fact together.
Captured lifecycle bytes are read only from the sealed private snapshot tree.
The live collection is never reopened.

# ERRORS

Projection failures throw *pkgsource::plan_adapter::projection_error*. Its
*projection_error_code* distinguishes captured lifecycle read failures from
planner fact rejection. Diagnostic text is not a stable protocol.

# BOUNDARIES

The adapter does not inspect package archives, issue artifact or artifact-
manifest identities, resolve dependency closure, choose path policy, observe
the target filesystem, or construct install and upgrade requests.

The core *libpkgsource* library remains independent of *libpkgplan*.

# SEE ALSO

*libpkgsource*(3), *pkgsource_snapshot*(3), *libpkgplan*(3)
