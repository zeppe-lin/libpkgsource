# Testing

## Purpose

The suite is organized by authority and failure attribution. A failure should
name the broken contract without requiring inspection of a monolithic test
program.

## Core behavior

`tests/unit/` covers canonical value admission, requirement scopes and subjects,
profile sealing and expansion, normalized recipe closure, and authentication of
public sealed-value reconstruction constructors.

`tests/integration/` composes the complete semantic pipeline. It proves source
identity field sensitivity and verifies that reconstructed source snapshots
must carry the identity of their exact sealed recipe while retaining diagnostic
source origin outside semantic identity.

## Codec behavior

`tests/protocol/` owns schema-one golden vectors, profile and source record
round-trips, envelope refusal, stored-identity substitution, invalid owner
values, and noncanonical representations. The fixed vectors are release
artifacts. Different bytes require an explicit record-schema decision.

## Internal provider behavior

`tests/mechanism/sha256_test.cpp` binds standard digest vectors and provider
state transitions. Source contracts prove that OpenSSL vocabulary exists only
inside the selected provider translation unit.

## Public and ABI behavior

`tests/header/` compiles every component header and both umbrella headers
independently. Shared builds compare dynamic exports with the reviewed core and
codec manifests. Generated metadata tests bind public and private dependency
placement without duplication.

`tests/installed/` contains consumers used only after installation. They execute
real source sealing and codec operations so shared/static pkg-config
qualification cannot pass by merely including headers; static qualification
therefore proves the private `libcrypto` closure and the codec's exact core
requirement.

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
- compile and execute installed core and codec consumers through pkg-config;
- regenerate and lint all manuals;
- build strict Doxygen and versioned HTML output;
- stage canonical, man, and HTML installations through `DESTDIR`;
- replay the complete patch series on the recorded base.
