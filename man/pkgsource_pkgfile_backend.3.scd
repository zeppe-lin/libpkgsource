pkgsource_pkgfile_backend(3)

# NAME

pkgsource_pkgfile_backend - complete pkgfile/0 source-directory backend

# SYNOPSIS

*#include <libpkgsource/pkgfile_backend.h>*

*pkgsource::pkgfile_backend backend;*

# DESCRIPTION

*pkgfile_backend* implements *source_format::pkgfile_v0*. It treats the complete
package directory as the legacy protocol: Pkgfile, metadata comments, shell
evaluated identity and source declarations, build entry point, .md5sum,
.footprint, .nostrip, .32bit, README resources, lifecycle programs, and all
sibling files visible to executable shell code.

The default constructor uses the private installed
*pkgsource-pkgfile-worker*. The path-taking constructor is intended for tests
and controlled embedding.

The worker sources the captured Pkgfile and returns a NUL-framed internal
record. C++ does not parse shell assignments. The directory basename must equal
the evaluated package name and *build()* must exist.

The legacy *Depends on:* metadata field becomes dependencies with
*build_and_run* scope. Every evaluated source local name must have exactly one
MD5 entry; unrelated entries are rejected. The backend does not verify remote
contents.

.footprint is exposed as a typed captured declaration and is not parsed in
version 0.1.0.

# SECURITY

Safe internal symbolic links are captured. Absolute, dangling, cyclic, and
escaping links, races, and special objects are rejected.

# SEE ALSO

*libpkgsource*(3), *pkgsource_backend*(3), *pkgsource_snapshot*(3)
