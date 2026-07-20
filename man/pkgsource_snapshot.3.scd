pkgsource_snapshot(3)

# NAME

pkgsource_snapshot - lifetime-owned immutable source capture

# SYNOPSIS

*#include <libpkgsource/snapshot.h>*

# DESCRIPTION

*source_snapshot* stores the original *source_location*, typed source format,
SHA-256 source-protocol fingerprint, normalized *build_description*, and shared
ownership of a private captured tree.

For *pkgfile/0*, the complete package-source directory is copied. Regular files
are read through non-following descriptors, symbolic links are validated to
remain inside the source root, unsupported objects are rejected, and both the
original and captured trees are rescanned for mutation. Directory modes are
restored after copying, so the inspector's ambient umask does not change the
fingerprint.

Before return, regular files are sealed owner-read-only while preserving their
original executable indication; directories are sealed owner-read/execute.
The native root is removed after the final snapshot or *captured_file* owner is
destroyed.

# FINGERPRINT

The deterministic fingerprint includes relative paths, entry types, permission
modes, regular-file sizes and contents, and symbolic-link targets. It excludes
the random private path, uid, gid, inode numbers, and timestamps.

*native_root()* and *captured_file::native_path()* expose filesystem paths for
future adapters. Callers must treat them as read-only.

# SEE ALSO

*libpkgsource*(3), *pkgsource_model*(3), *pkgsource_backend*(3)
