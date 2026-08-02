# Architecture

## Authority boundary

Input syntax is not authority. Frontends may construct parser-neutral
declarations, but only core sealers produce owner values.

```text
syntax adapter or importer
          |
          v
parser-neutral declarations
          |
          +--> profile_catalog::seal()
          |
          +--> seal_source()
                  |
                  v
          immutable owner values
```

The semantic core has no parser, collection, planner, execution, state, or
storage dependency. The codec has no syntax, catalog, planner, execution, or
store-policy dependency.

## Repository layout

```text
include/libpkgsource/          installed semantic API
include/libpkgsource-codec/    installed durable-record API
src/                           semantic implementation
codec/                         durable record implementation
internal/                      shared private SHA-256 provider
abi/                           reviewed dynamic-symbol manifests
tests/                         focused executable contracts
docs/                          canonical project knowledge
```

The two installed libraries are separate public products. Private source files
are grouped by responsibility rather than by their pre-split history.

## Semantic pipeline

```text
validated value domains
        |
        v
profile sealing and requirement expansion
        |
        v
recipe normalization and closure invariants
        |
        v
source-snapshot identity
```

Profiles are sealed values, not parser aliases. A sealed profile retains direct
members, exact transitive expansion paths, declaration provenance, and a
semantic identity. Nested profile identity contributes to parent identity.

Requirements preserve independent build, run, check, and exact lifecycle-action
scopes. Lifecycle requirements require a program for the same action. Check
requirements require a check program. A check program without additional check
requirements is valid.

## Identity boundary

A source snapshot retains diagnostic source origin, one complete sealed recipe,
and one domain-separated identity. Source origin and declaration provenance do
not participate in semantic identity.

The complete current model uses
`libpkgsource/source-snapshot/v1`. Package-release and profile identities use
independent version-one domains.

Identity framing is private to `src/internal/identity_writer.*`. Changing an
algorithm, domain, tag, field order, normalization rule, or participating field
is a protocol change.

## SHA-256 provider boundary

Both public libraries use the same private provider under `internal/`. Only the
provider translation unit knows OpenSSL types and return conventions. Public
headers and semantic framing contain no provider-specific types.

Switching to another qualified SHA-256 implementation must preserve all identity
and record bytes. Replacing SHA-256 requires new protocol versions; it is not a
build-option substitution.

## Codec ownership

Durable source records belong to the source owner. A generic evidence store may
retain, address, index, or garbage-collect the bytes, but it does not define
source schemas or reconstruct source semantics.

```text
libpkgsource.so.3
    semantic declarations, sealing, identities

libpkgsource-codec.so.1
    canonical records, bounded decoding, resealing verification
```

The core has no reverse dependency on the codec. A source record embeds only the
retained profile closure needed to reconstruct that snapshot. It does not embed
an unrelated acquired catalog and does not invent a store-level reference
protocol.

## Decode discipline

A record is accepted only when:

1. size, magic, version, framing, tags, and checksum are valid;
2. owner constructors and sealers accept every reconstructed value;
3. stored identities equal recomputed identities;
4. canonical re-encoding exactly reproduces the input bytes.

Checksum validity alone is not semantic validity. Successful resealing alone is
not canonicality.

## Installed documentation

Canonical Markdown installs under `share/doc/libpkgsource`. Generated man pages
install under the ordinary man hierarchy. Build metadata and committed derived
roff do not escape into the canonical documentation tree.

## HTML publication boundary

When explicitly enabled, this repository renders a versioned static tree under
`share/htmldocs/libpkgsource/<version>`. The project site may publish that tree
unchanged. It does not rerun Doxygen or Pandoc and does not become a second
source of truth.
