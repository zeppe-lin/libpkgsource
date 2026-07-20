pkgsource_backend(3)

# NAME

pkgsource_backend - package-source format backend contract

# SYNOPSIS

*#include <libpkgsource/backend.h>*

# DESCRIPTION

*source_backend* is the abstract inspection interface.

*format()* returns the backend's typed source format. *probe()* tests whether one
source location is recognized. *inspect()* captures and validates one source
directory and returns a *source_snapshot*.

*inspect_request* contains a *source_location*, optional private snapshot parent,
and an *evaluation_policy*.

*evaluation_policy* contains an explicit environment map, optional
*worker_identity*, and a file-creation mask. Ambient process environment is not
inherited by the Pkgfile worker. Unsafe loader and shell-startup variables are
rejected. The worker runs in a private process group; remaining descendants are
terminated after the evaluation shell exits.

A backend never walks collection roots or applies package precedence.

# ERRORS

*inspect()* throws *pkgsource::error* for invalid requests, unsafe trees,
mutation, worker failures, malformed records, invalid format semantics, or
filesystem failures.

# SEE ALSO

*libpkgsource*(3), *pkgsource_pkgfile_backend*(3), *pkgsource_snapshot*(3)
