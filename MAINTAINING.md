<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Maintaining

## Release authority

A release is defined by one signed repository tag. `libpkgsource` and
`libpkgsource-codec` share the project version because the durable codec knows
the exact owner model. Their SONAMEs remain independent.

The release candidate must pass GCC and Clang shared/static builds, warnings as
errors, ASan and UBSan, generated pkg-config checks, installed-consumer checks,
manual rendering and lint, shared-object dependency inspection, and exact
`git am` replay.

## Identity discipline

Package-release, profile, and source-snapshot identities are owner protocols.
Changing field participation, normalization, tag values, or domain strings is a
protocol change. Update fixed vectors and document whether old identities remain
admissible; never silently reinterpret them.

## Durable records

`docs/protocols/source-records-v1.md` is normative. Schema 1 accepts no trailing or unknown
fields. Any change to emitted or accepted bytes requires either proof that the
schema-1 bytes are unchanged or a new outer schema version with new vectors.

The intrinsic checksum, semantic resealing, stored-identity comparison, and
canonical re-encoding are separate checks. Do not remove one because a future
store also content-addresses records.

## Downstream order

Publish the core tag before releasing `libpkgsource-yaml` or
`libpkgsource-plan` versions that require it. Then rebuild direct consumers such
as catalog acquisition and orchestration against the signed tags. Do not qualify
those consumers against uncommitted local headers.
