<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# profiles.yml/1 input contract

`profiles.yml/1` is the native syntax for authoritative named requirement
profiles.  It is an input protocol, not profile authority.  A reader translates
one document into parser-neutral `profile_declaration` values and passes them to
`profile_catalog::seal()`.  Only the returned `profile_catalog` and its
`sealed_profile` values are authoritative.

## Document shape

```yaml
format: zeppe-lin.profiles/1

profiles:
  compiler:
    members:
      - package: gcc

  toolchain:
    members:
      - profile: "@compiler"
      - package: binutils
      - package: make
```

`format` and `profiles` are required.  `format` must be exactly
`zeppe-lin.profiles/1`.  `profiles` is a mapping and may be empty.

Profile definition keys omit the leading `@`.  The parser adds it before
constructing the exact native `profile_reference`.  Names therefore use the
same canonical lowercase ASCII identity rules as the core model and cannot be
aliases or alternate spellings.

Every profile definition contains exactly one required `members` sequence.  It
must be non-empty.  Each member is an explicit one-key mapping:

```yaml
- package: pkg-config
- profile: "@toolchain"
```

Scalar shorthand is not accepted.  Package and profile subjects remain
separate authority domains.  A nested profile reference includes its leading
`@` and must resolve in the complete document when the profile catalog is
sealed.

## Strict YAML subset

Version one accepts one YAML document.  Duplicate mapping keys, unknown keys,
non-scalar mapping keys, YAML directives, anchors, aliases, merge keys, and
custom or kind-incompatible tags are rejected.  Standard string, integer,
sequence, and mapping tags are accepted only where their node kind matches the
schema.  No YAML scalar typing is used to infer package semantics.

Mapping and sequence order are retained only as declaration provenance.  The
native profile sealer normalizes definitions and members, rejects duplicate
definitions and direct members, rejects unknown profiles and cycles, expands
nested profiles deterministically, and computes domain-separated identities.

Document names, mapping paths, and one-based line and column positions are
retained for diagnostics but excluded from semantic identity.

## Authority boundary

`libpkgsource-yaml` parses document bytes.  It does not open files, walk
collections, merge profile documents, choose collection precedence, or mutate a
previously sealed profile catalog.  A future acquisition frontend will define
how multiple collection documents are gathered and will present one explicit
global declaration set to the sealer.
