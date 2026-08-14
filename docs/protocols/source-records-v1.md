# libpkgsource durable records, schema 1

This document normatively defines the canonical byte representation emitted by
`libpkgsource-codec.so.2` for `libpkgsource >= 4.1.0, < 5.0.0` owner authority.

Two record types are defined:

- profile-catalog record, magic `ZLPSPCAT`, schema 1;
- source-snapshot record, magic `ZLPSSNAP`, schema 1.

The words MUST, MUST NOT, REQUIRED, SHALL, SHALL NOT, SHOULD, SHOULD NOT, and MAY
are normative.

## Primitive encodings

All integers are unsigned and big-endian.

- `u8`, `u16`, `u32`, `u64`: exactly 1, 2, 4, or 8 bytes.
- `text`: `u32 byte_count`, followed by exactly that many bytes. It is not
  NUL-terminated. Semantic constructors define any character restrictions.
- `blob`: `u64 byte_count`, followed by exactly that many bytes.
- `presence`: one `u8`, exactly 0 or 1.
- `count`: one `u32`, not greater than 1,000,000.
- `checksum`: the final 32 bytes, equal to SHA-256 over every preceding byte in
  the outer record.

No alignment, padding, extension area, trailing bytes, or implicit field is
permitted.

The maximum complete profile-catalog record size is 64 MiB. The maximum complete
source-snapshot record size is 128 MiB. The checksum is included in those
limits. An embedded profile record remains subject to the 64 MiB profile limit.

## Common tagged values

### Provenance

```text
text document
text path
u32  line
u32  column
```

### Requirement subject

```text
u8   kind       # 1 package, 2 profile
text value
```

### Requirement scope

```text
u8 kind         # 1 build, 2 run, 3 check, 4 lifecycle
[when kind=4]
  u8 action     # 1 pre-install, 2 post-install,
                # 3 pre-remove, 4 post-remove
```

### Program

```text
u8   language   # 1 posix-shell
text material
```

### Lifecycle program

```text
u8 action       # same action tags as above
program value
```

### Source input

```text
u8   kind       # 1 remote, 2 local
text location
text local_name
u8   unpack     # 1 none, 2 archive
u8   digest     # 1 sha256
text digest_hex
```

### Architectures

```text
count build_count
text  build[build_count]
count target_count
text  target[target_count]
```

### Package metadata

```text
text     summary
presence description_present
[text description]
presence homepage_present
[text homepage]
count    license_count
text     licenses[license_count]
```

## Profile-catalog record

```text
byte[8] magic = "ZLPSPCAT"
u16     schema = 1
count   profile_count
profile profiles[profile_count]
checksum
```

Each profile is:

```text
text       canonical_profile_name
text       stored_profile_identity_hex
provenance declaration
count      direct_member_count
member     direct_members[direct_member_count]
```

Each direct member is:

```text
requirement_subject subject
provenance          declaration
```

Profiles and direct members MUST occur in the exact normalized order returned by
`profile_catalog::seal()`.

Decoding MUST reconstruct `profile_declaration` values, invoke
`profile_catalog::seal()`, compare every recomputed profile identity with the
stored identity at the same normalized position, and require exact re-encoding.

## Source-snapshot record

```text
byte[8] magic = "ZLPSSNAP"
u16     schema = 1
text     source_origin_document
text     stored_source_snapshot_identity_hex
blob     retained_profile_catalog_record

text     package_name
text     package_version
u32      package_release
metadata package_metadata

count    source_count
source   sources[source_count]
program  build_program
presence check_program_present
[program check_program]

count       requirement_declaration_count
requirement requirement_declarations[requirement_declaration_count]

count             lifecycle_program_count
lifecycle_program lifecycle_programs[lifecycle_program_count]

architectures architecture_requirements
provenance    recipe_declaration
checksum
```

Each requirement declaration is:

```text
requirement_scope   scope
requirement_subject subject
provenance          declaration
```

The embedded profile-catalog record MUST contain exactly the profile closure
retained by the sealed recipe, reconstructed as direct profile declarations. It
MUST NOT include unrelated profiles from an acquired global catalog.

The requirement declaration sequence is reconstructed from the complete sealed
requirement-origin set. One declaration appears for each unique `(scope,
subject)` pair. Its provenance is the unique provenance retained for that direct
or selected-profile declaration. Contradictory provenance for the same pair is
not encodable.

Sources, requirement declarations, lifecycle programs, architecture values, and
embedded profiles MUST occur in the exact normalized order produced by the
owner model and codec.

Decoding MUST:

1. decode and verify the embedded profile record;
2. reconstruct one complete `recipe_declaration`;
3. invoke `seal_source()` with the decoded source origin and profile catalog;
4. compare the recomputed source-snapshot identity with the stored identity; and
5. require exact source-record re-encoding.

The record contains no syntax tag, YAML version, recipe identity, collection
path, repository precedence, resolved dependency, fetched object, execution
result, planner fact, or state publication.

## Canonicality

A checksum-valid record is still invalid unless owner reconstruction succeeds.
A semantically valid record is still noncanonical unless re-encoding produces
exactly the original bytes.

The decoder rejects:

- records over fixed limits;
- truncated records;
- wrong magic;
- unsupported schema versions;
- checksum mismatch;
- unknown tags or invalid presence flags;
- invalid owner values or failed sealing;
- stored identity substitution;
- trailing fields; and
- any alternate byte representation of the same owner value.

Schema 1 has no unknown-field rule. A decoder MUST NOT skip or preserve trailing
fields. Evolution requires a new outer schema version.

## Diagnostics and semantic identity

Source origin and declaration provenance are checksum-protected record content,
but they do not contribute to source semantic identity. Therefore two snapshots
with equal semantic authority and different diagnostic locations have equal
source identities but distinct durable record bytes.

This distinction is intentional. The owner record retains complete reconstructive
and diagnostic evidence; semantic identity answers whether package-source
meaning is equal.

## Golden vectors

The following vectors bind schema 1 independently of C++ object layout. The
reported digest is SHA-256 over the complete encoded record, including its
intrinsic final checksum.

### Empty profile catalog

Semantic value: `profile_catalog::seal({})`.

```text
complete record length: 46 bytes
complete record SHA-256:
  2f268947090f17e2c4f1825c0c7167930c8950327c6389c6531d8e6f64b4e483
```

### Minimal source snapshot

Semantic value:

```text
origin document: recipe.yml
package: a
version: 1
release: 1
summary: A
description: absent
homepage: absent
licenses: MIT
sources: empty
build program: posix-shell, exact bytes "true\n"
check program: absent
requirements: empty
lifecycle programs: empty
build architectures: empty
target architectures: empty
recipe provenance: document=recipe.yml, path=document, line=1, column=1
retained profile catalog: empty
```

```text
source snapshot identity:
  485b072c47308c1c64e8ec9d8c88c2418c57642c2b50e4e59c853c509d5da838
complete record length: 275 bytes
complete record SHA-256:
  cd221e9527162de41fa23806f2a370e161139cf059c6dc77d08cbfd37b45be35
```

`tests/protocol/golden_vectors_test.cpp` reconstructs these values through the public owner API
and asserts the vectors. A schema implementation that emits different bytes is
not schema-1 compatible.

## Checksum and external content address

The intrinsic checksum is part of the owner protocol. A storage system MAY also
content-address the complete record. The external digest does not replace the
intrinsic checksum, owner resealing, stored-identity comparison, or canonical
re-encoding.
