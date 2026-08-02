pkgsource_codec(3)

# NAME

pkgsource_codec - canonical durable package-source authority

# SYNOPSIS

*#include <libpkgsource-codec/codec.h>*

*profile_catalog_encoding encode_profile_catalog(const profile_catalog& catalog);*

*profile_catalog decode_profile_catalog(const profile_catalog_encoding& encoding);*

*source_snapshot_encoding encode_source_snapshot(const source_snapshot& snapshot);*

*source_snapshot decode_source_snapshot(const source_snapshot_encoding& encoding);*

# DESCRIPTION

The profile-catalog and source-snapshot codecs are the durable representation
owned by *libpkgsource-codec*.  Records use fixed eight-byte house magics, a
big-endian schema version, bounded length-prefixed fields, and a whole-record
SHA-256 checksum.

*decode_profile_catalog()* reconstructs parser-neutral profile declarations and
invokes *profile_catalog::seal()*.  Stored profile identities must equal the
identities recomputed by the sealer, and canonical re-encoding must reproduce
the input bytes.

*decode_source_snapshot()* reconstructs the retained profile closure, original
recipe requirement declarations, and the complete recipe declaration.  It then
invokes *seal_source()*.  Stored recipe and source-snapshot identities must equal
the recomputed identities, and canonical re-encoding must reproduce the input.

Source document origin and syntax are retained as checksum-protected diagnostic
provenance.  They do not contribute to recipe or source-snapshot semantic
identity.

The decoders do not parse YAML, open paths, discover collections, fetch source
objects, execute programs, project planner facts, or reconstruct downstream
state.

# ERRORS

*codec_error* reports stable *codec_error_code* values for size refusal,
truncation, invalid magic, unsupported versions, checksum mismatch, invalid
records, identity mismatch, and non-canonical input.

# SEE ALSO

*libpkgsource-codec*(3), *pkgsource_profile*(3), *pkgsource_recipe*(3),
*pkgsource_snapshot*(3)
