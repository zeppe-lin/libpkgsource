pkgsource_codec(3)

# NAME

pkgsource_codec - canonical durable package-source authority records

# SYNOPSIS

*#include <libpkgsource-codec/codec.h>*

*profile_catalog_encoding encode_profile_catalog(*
*    const profile_catalog& catalog);*

*profile_catalog decode_profile_catalog(*
*    const profile_catalog_encoding& encoding);*

*source_snapshot_encoding encode_source_snapshot(*
*    const source_snapshot& snapshot);*

*source_snapshot decode_source_snapshot(*
*    const source_snapshot_encoding& encoding);*

# DESCRIPTION

*libpkgsource-codec* is the durable-record sibling of *libpkgsource*.  It owns
canonical schema-one encodings for complete sealed profile catalogs and source
snapshots.

Records use fixed eight-byte magics, big-endian integers, bounded length-prefixed
fields, explicit tags, and a final SHA-256 checksum.  Profile records are limited
to 64 MiB, source records to 128 MiB, and every encoded collection to 1,000,000
items.

*decode_profile_catalog()* reconstructs direct profile declarations and invokes
*profile_catalog::seal()*.  Stored profile identities must match recomputed
identities.

*decode_source_snapshot()* reconstructs the exact retained profile closure and
one complete recipe declaration, then invokes *seal_source()*.  The stored
source identity must match the recomputed identity.

Both decoders require canonical byte-for-byte re-encoding.  A checksum-valid
alternate representation, trailing field, or reordered record is rejected.

The source record embeds only the profile closure retained by that snapshot.  It
does not embed an unrelated acquired global catalog and does not require a store
lookup for reconstruction.

# ERRORS

*codec_error* reports stable *codec_error_code* values for size refusal,
truncation, invalid magic, unsupported version, checksum mismatch, invalid
record structure or owner values, identity mismatch, and noncanonical input.

# BOUNDARY

The codec does not parse YAML, open paths, discover collections, fetch source
objects, execute programs, resolve dependencies, project planner facts, or
publish state.  A generic evidence store may retain codec bytes but does not own
their source schema.

# ABI AND PROTOCOL

The first codec ABI is *libpkgsource-codec.so.1*.  It requires the exact matching
*libpkgsource* project version.  The normative byte protocol is documented in
*SOURCE-RECORDS-1.md*.

# SEE ALSO

*libpkgsource*(3), *pkgsource_profile*(3), *pkgsource_recipe*(3),
*pkgsource_snapshot*(3)
