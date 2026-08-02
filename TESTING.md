<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Testing

The repository suite is offline and model-driven. It opens no package source
paths, parses no YAML, performs no network access, and executes no recipe
programs.

## Core model

The model, profile, and recipe tests cover:

- canonical package, profile, architecture, digest, path, and program values;
- package-release identity vectors;
- independent build, run, check, and lifecycle requirement scopes;
- exact package and profile subjects;
- deterministic profile sealing independent of declaration order;
- nested expansion, selected build profiles, retained closure, and origin paths;
- duplicate profile definitions or members, unknown profiles, and cycles;
- source ordering and duplicate local-name rejection;
- lifecycle program uniqueness and lifecycle requirement/program closure;
- optional check program retention and check requirement/program closure;
- independent build and target architecture sets;
- source identity stability across declaration order and diagnostic provenance;
- source identity sensitivity to every semantic field;
- the first public source-snapshot identity vectors; and
- independent core public-header compilation.

## Durable codec

The codec tests cover:

- deterministic profile-catalog bytes independent of declaration insertion
  order;
- source records with and without a check program;
- reconstruction of direct and profile-derived requirement declarations;
- exact retained profile closure reconstruction;
- canonical byte-for-byte round trips;
- fixed profile and source golden vectors;
- checksum corruption, truncation, bad magic, unsupported version, invalid tag,
  trailing field, embedded-record corruption, and stored-identity substitution;
- size and item-count refusal;
- rejection of noncanonical but checksum-valid encodings;
- independent codec public-header compilation; and
- static source checks for resealing, canonical re-encoding, limits, SONAME, and
  exact core-version coupling.

## Release matrix

Before release, run clean GCC and Clang configurations for both shared and
static libraries with warnings as errors. Run ASan and UBSan over every runtime
test. Render all scdoc pages and lint them with mandoc.

Inspect:

- `libpkgsource.so.3` and `libpkgsource-codec.so.1` SONAMEs;
- shared `NEEDED` closure;
- `libpkgsource.pc` and `libpkgsource-codec.pc` public/private requirements;
- `pkg-config --static` closure;
- independently compiled installed consumers for both headers; and
- exact `git am` replay tree identity.

The codec protocol specification and golden vectors are release artifacts. A
change to accepted bytes requires an explicit new record schema version; a
change to source semantics requires an explicit source identity decision.
