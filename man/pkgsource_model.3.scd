pkgsource_model(3)

# NAME

pkgsource_model - normalized package-source value model

# SYNOPSIS

*#include <libpkgsource/model.h>*

# DESCRIPTION

The model consists of immutable classes with construction-time values and const
accessors.

*package_identity* stores package name, version, and release.

*descriptive_metadata* stores optional description, URL, packager, and
maintainer fields.

*dependency* stores a name and typed scope. For *pkgfile/0*, the legacy
*Depends on:* field maps to *build_and_run*. This is a compatibility mapping,
not the final dependency taxonomy.

*source_input* stores the evaluated declaration, remote or recipe-local kind,
local name, optional remote locator, declared digests, and an optional captured
local file.

*recipe_descriptor* binds the *build* entry point to the captured Pkgfile.
*lifecycle_action* maps legacy action filenames to typed phases and captured
programs. *resource* describes README files. *strip_exclusion* contains a POSIX
extended regular expression. *footprint_declaration* binds a captured legacy
footprint without interpreting it as a package image.

*build_description* aggregates the normalized model and the native or legacy
32-bit architecture marker.

# FILE REFERENCES

*captured_file* stores a relative path, original mode, size, SHA-256 content
digest, and shared ownership of the private snapshot tree. *native_path()* is
valid while any copy of the captured file or parent snapshot remains alive.

# SEE ALSO

*libpkgsource*(3), *pkgsource_snapshot*(3)
