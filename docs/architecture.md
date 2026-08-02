<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Design

## Owner model

`libpkgsource` owns native package-source semantics. A parser may construct
unsealed declarations, but only the core sealers may turn those declarations
into authority.

```text
syntax adapter                 semantic owner
--------------                 --------------
bytes -> declarations  ----->  profile_catalog::seal()
                               seal_source()
                                      |
                                      v
                            immutable owner values
```

The core has no YAML, collection, planner, execution, or storage dependency.

## Core boundary

The core owns:

- canonical package, profile, and architecture value domains;
- package release and metadata values;
- exact source declarations and program material;
- typed requirement scope and subject domains;
- profile declarations, deterministic expansion, and profile identities;
- parser-neutral recipe declarations;
- source normalization, closure invariants, and source-snapshot identity.

It does not own input grammar, collection acquisition, dependency resolution,
source materialization, build/check/lifecycle execution, transaction planning,
state publication, or evidence-store layout.

## Profiles and requirements

Profiles are sealed values, not parser aliases. A sealed profile retains its
canonical name, direct members, transitive expansion paths, edge provenance,
and identity. Nested profile identity contributes to parent identity.

Recipe requirements retain independent build, run, check, and exact lifecycle
action scopes. Package and profile subjects are separate domains. Expansion
produces exact package requirements while retaining every direct or profile
origin, selected build-profile roots, and the complete profile closure used by
the source snapshot.

Lifecycle requirements require a program for the same action. Check
requirements require a check program. A check program without additional check
requirements is valid.

## Source identity

A `source_snapshot` contains diagnostic `source_origin`, one complete
`sealed_recipe`, and one domain-separated source identity. Origin and
provenance do not contribute to semantic identity.

Version 3 resets the complete current model to the first public
`libpkgsource/source-snapshot/v1` domain. This is intentional: the earlier
recipe/syntax generations were published repository experiments but had not
been admitted as package-system evidence. Preserving them would manufacture a
compatibility obligation before the evidence store existed.

Package-release and profile identity domains remain at version one because
their semantic contracts did not split into competing pre-release generations.

## Codec ownership

Durable source records belong to the source owner. A generic store may retain,
address, index, or garbage-collect them, but it must not define source schemas or
reconstruct source semantics itself.

The codec therefore remains in this repository while being isolated as a
separate library:

```text
libpkgsource.so.3
    semantic declarations, sealing, identities

libpkgsource-codec.so.1
    canonical records, bounded decoding, resealing verification
```

The codec depends on the exact matching `libpkgsource` project version. The core
has no reverse dependency on the codec.

## Self-contained source records

A source record embeds the exact retained profile closure needed to reconstruct
that snapshot. It does not embed the complete acquired profile universe and it
does not depend on an external store lookup.

This is deliberate for the first owner protocol:

- decoding is independently deterministic;
- transport outside `pkgctl` remains possible;
- missing external references cannot silently change reconstruction;
- no store-level graph protocol is invented before the store owns one.

A future store may deduplicate complete owner records by content address. That
is a storage optimization, not a reason to weaken the primitive owner record.

## Decode path

Profile decoding reconstructs direct profile declarations and calls
`profile_catalog::seal()`. Source decoding reconstructs the retained closure and
recipe declaration and calls `seal_source()`.

A record is accepted only when:

1. size, magic, schema version, structure, tags, and checksum are valid;
2. owner constructors and sealers accept every reconstructed value;
3. stored identities equal recomputed identities; and
4. canonical re-encoding exactly reproduces the input bytes.

The last check rejects alternate field ordering, redundant encodings, trailing
fields, or any other byte representation that maps to the same semantic value.

## Record checksum versus store address

The record checksum is intrinsic owner-level corruption detection. A future
content-addressed store may also hash the complete record to name or verify the
stored object. These protections are intentionally distinct:

- the codec checksum makes the record independently verifiable in transit;
- the store digest binds storage coordinates and store policy.

Neither digest substitutes for semantic resealing and identity verification.
