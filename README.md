<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# libpkgsource

`libpkgsource` is the native Zeppe-Lin C++17 package-source authority.

The core library accepts parser-neutral declarations, seals requirement
profiles, expands exact package requirements, validates complete recipe
semantics, and returns immutable normalized source snapshots. Input syntax is
not authority. The sealed `source_snapshot` is.

The repository also owns `libpkgsource-codec`, a separate sibling library that
encodes and decodes canonical durable records for sealed profile catalogs and
source snapshots. The codec belongs to the source owner because it knows every
source field, invariant, identity, and resealing obligation, but it is not part
of the semantic core ABI.

YAML parsing and planner projection now live in independent repositories:

- `libpkgsource-yaml`: strict YAML bytes to parser-neutral declarations;
- `libpkgsource-plan`: sealed source snapshot to `libpkgplan` candidate facts.

## Core authority

The normalized source model distinguishes:

- package release and metadata;
- remote and local source declarations with SHA-256 content requirements;
- exact build and optional check program bytes;
- build, run, check, and action-bound lifecycle requirements;
- exact package and named-profile subjects;
- selected build profiles and the complete retained profile closure;
- installation and removal lifecycle programs;
- independent build and target architecture constraints; and
- package-release, profile, and source-snapshot identity domains.

`profile_catalog::seal()` owns deterministic profile normalization, nested
expansion, duplicate and cycle rejection, retained expansion provenance, and
profile identities.

`seal_source()` owns source normalization, profile expansion, duplicate source
and lifecycle detection, lifecycle requirement/program closure, check
requirement/program closure, and the source-snapshot identity.

The current complete source model uses the first public
`libpkgsource/source-snapshot/v1` identity domain. Earlier source-snapshot
identity generations belonged to the pre-package development line and are not
accepted as durable evidence by 3.0.

## Durable owner records

`libpkgsource-codec.so.1` provides:

```cpp
#include <libpkgsource-codec/codec.h>

auto profile_record = pkgsource::codec::encode_profile_catalog(catalog);
auto source_record = pkgsource::codec::encode_source_snapshot(snapshot);

auto restored_catalog =
    pkgsource::codec::decode_profile_catalog(profile_record);
auto restored_snapshot =
    pkgsource::codec::decode_source_snapshot(source_record);
```

Decoding never trusts bytes as authority. It reconstructs declarations, invokes
the ordinary owner sealers, verifies stored identities, and requires canonical
byte-for-byte re-encoding. Records are bounded, versioned, big-endian, and
protected by an internal SHA-256 checksum.

A source record embeds only the exact profile closure retained by that snapshot,
not an acquired global catalog. This keeps the primitive record independently
reconstructible without duplicating unrelated profile authority.

The normative byte protocol is `docs/protocols/source-records-v1.md`.

## Boundary

Neither library:

- parses YAML or any other source syntax;
- opens source paths or discovers package collections;
- chooses collection precedence;
- resolves package availability or dependency closure;
- downloads or verifies source objects;
- executes build, check, or lifecycle programs;
- creates package images or archives;
- installs packages or reads installed state;
- projects planner facts; or
- imports Pkgfile or historical package-database state.

Those are separate owners and explicit adapters.

## Build

Shared and static libraries require separate Meson configurations.
`default_library=both` is rejected and `link_mode` must match the selected
library kind.

```sh
meson setup build-shared \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
  -Dman_pages=enabled
meson compile -C build-shared
meson test -C build-shared --print-errorlogs

meson setup build-static \
  -Ddefault_library=static \
  -Dlink_mode=static \
  -Dman_pages=enabled
meson compile -C build-static
meson test -C build-static --print-errorlogs
```

The release installs `libpkgsource.so.3`, `libpkgsource-codec.so.1`, and their
separate pkg-config modules.

GPL-3.0-or-later. See `COPYING` and `COPYRIGHT`.
