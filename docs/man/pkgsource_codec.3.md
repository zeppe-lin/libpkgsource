% PKGSOURCE_CODEC(3) libpkgsource 3.0.0 | libpkgsource

# NAME

pkgsource_codec - encode canonical durable source-authority records

# SYNOPSIS

```cpp
#include <libpkgsource-codec/libpkgsource-codec.h>

pkgsource::codec::profile_catalog_encoding
pkgsource::codec::encode_profile_catalog(
    const pkgsource::profile_catalog& catalog);

pkgsource::profile_catalog pkgsource::codec::decode_profile_catalog(
    const pkgsource::codec::profile_catalog_encoding& encoding);

pkgsource::codec::source_snapshot_encoding
pkgsource::codec::encode_source_snapshot(
    const pkgsource::source_snapshot& snapshot);

pkgsource::source_snapshot pkgsource::codec::decode_source_snapshot(
    const pkgsource::codec::source_snapshot_encoding& encoding);
```

# DESCRIPTION

`libpkgsource-codec` is the durable-record sibling of `libpkgsource`. It owns
canonical schema-one encodings for complete sealed profile catalogs and source
snapshots.

Records use fixed eight-byte magics, big-endian integers, bounded
length-prefixed fields, explicit tags, and a final SHA-256 checksum. Profile
records are limited to 64 MiB, source records to 128 MiB, and each encoded
collection to 1,000,000 items.

`decode_profile_catalog()` reconstructs direct profile declarations and invokes
`pkgsource::profile_catalog::seal()`. Stored profile identities must match
recomputed identities.

`decode_source_snapshot()` reconstructs the exact retained profile closure and
one complete recipe declaration, then invokes `pkgsource::seal_source()`. The
stored source identity must match the recomputed identity.

Both decoders require canonical byte-for-byte re-encoding. A checksum-valid
alternate representation, trailing field, or reordered record is rejected.

# ERRORS

`pkgsource::codec::codec_error` reports stable
`pkgsource::codec::codec_error_code` values for size refusal, truncation,
invalid magic, unsupported version, checksum mismatch, invalid record material,
identity mismatch, and noncanonical input.

# ABI AND PROTOCOL

The codec ABI is `libpkgsource-codec.so.1`. The normative byte protocol is
`docs/protocols/source-records-v1.md`.

# SEE ALSO

`libpkgsource(3)`, `pkgsource_profile(3)`, `pkgsource_recipe(3)`,
`pkgsource_snapshot(3)`
