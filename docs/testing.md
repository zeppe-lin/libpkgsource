# Testing

## Purpose

The suite is organized by authority and failure attribution. A failure should
name the broken contract without requiring inspection of a monolithic test
program.

## Core behavior

`tests/model/` covers canonical values, scopes, subjects, metadata, source
locations, programs, architectures, and identity validation.

`tests/profiles/` covers deterministic expansion, requirement provenance,
selected profiles, retained closure, identity participation, duplicates,
unknown profiles, and cycles.

`tests/recipes/` covers normalized content, declaration-order independence,
identity participation, check-program authority, program digest vectors, and
closure failures.

## Codec behavior

`tests/codec/` separates golden vectors, profile records, source records,
envelope failures, stored-identity substitution, invalid owner values, and
noncanonical representations. The fixed schema-one vectors are release
artifacts. Different bytes require an explicit record-schema decision.

## Internal provider behavior

`tests/internal/sha256_test.cpp` binds standard digest vectors and provider
state transitions. Source contracts prove that OpenSSL vocabulary exists only
inside the selected provider translation unit.

## Public and ABI behavior

Every component header and both umbrella headers compile independently. Shared
builds compare dynamic exports with the reviewed core and codec manifests.
Generated metadata tests bind public and private dependency placement without
duplication. Installed-consumer tests cover shared and static pkg-config
closures.

## Documentation behavior

Manual source restrictions, Pandoc writer compatibility, canonicalized roff,
Doxygen completeness, HTML links, and staged `DESTDIR` installation are
executable contracts. Ordinary builds install committed roff and canonical
Markdown without requiring documentation generators.

## Release qualification

Before publication:

- run GCC and Clang shared and static configurations with warnings as errors;
- run ASan and UBSan under both compilers;
- inspect SONAMEs, `NEEDED` closure, and exact exports;
- compile installed core and codec consumers through pkg-config;
- regenerate and lint all manuals;
- build strict Doxygen and versioned HTML output;
- stage canonical, man, and HTML installations through `DESTDIR`;
- replay the complete patch series on the recorded base.
