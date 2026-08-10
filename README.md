# libpkgsource

`libpkgsource` is the native Zeppe-Lin C++17 package-source authority.

The semantic core accepts parser-neutral declarations, validates canonical
value domains, seals requirement profiles, expands exact package requirements,
normalizes complete recipe semantics, and returns immutable source snapshots.
Input syntax is not authority. A declaration becomes authority only through the
owner sealers. Public reconstruction constructors authenticate the canonical
sealed authority supplied to them; they do not permit callers to mint arbitrary
"sealed" identities or noncanonical recipe state.

The repository also owns `libpkgsource-codec`, a separately linked sibling
library for canonical durable records. The codec remains beside the semantic
owner because decoding must reconstruct declarations, invoke the ordinary
sealers, verify stored identities, and reject noncanonical bytes. It is not part
of the core ABI.

YAML parsing and planner projection live in independent repositories:

- `libpkgsource-yaml`: strict YAML bytes to parser-neutral declarations;
- `libpkgsource-plan`: sealed source snapshots to `libpkgplan` candidate facts.

## Public libraries

`libpkgsource.so.3` owns:

- package, profile, architecture, digest, provenance, and program value domains;
- package releases, metadata, source inputs, and requirement declarations;
- deterministic profile sealing and transitive expansion;
- normalized recipe authority and closure invariants;
- package-release, profile, and source-snapshot semantic identities.

Use the umbrella header:

```cpp
#include <libpkgsource/libpkgsource.h>
```

`libpkgsource-codec.so.1` owns:

- schema-one profile-catalog and source-snapshot records;
- bounded big-endian framing and intrinsic SHA-256 checksums;
- reconstruction through core constructors and sealers;
- stored-identity verification and byte-for-byte canonicality checks.

Use its separate umbrella header:

```cpp
#include <libpkgsource-codec/libpkgsource-codec.h>
```

The normative record protocol is
`docs/protocols/source-records-v1.md`.

## Authority boundary

Neither library parses YAML, opens package-source paths, discovers collections,
chooses precedence, resolves dependency closure, fetches objects, executes
programs, creates package images, installs packages, reads installed state,
projects planner facts, or publishes transaction evidence.

The core has no dependency on the codec. Both libraries use one private
repository-owned SHA-256 provider boundary; no provider type appears in either
public API.

## Build

Meson 1.2 or newer is required. Shared and static builds use separate Meson
configurations. `link_mode` must
match `default_library`; `default_library=both` is rejected.

```sh
meson setup build-shared \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
  -Dtests=enabled \
  -Dman_pages=enabled \
  -Dhtml_docs=disabled \
  -Dwerror=true
meson compile -C build-shared
meson test -C build-shared --print-errorlogs

meson setup build-static \
  -Ddefault_library=static \
  -Dlink_mode=static \
  -Dtests=enabled \
  -Dman_pages=disabled \
  -Dhtml_docs=disabled \
  -Dwerror=true
meson compile -C build-static
meson test -C build-static --print-errorlogs
```

## Installed documentation

Canonical Markdown and project policy install under:

```text
${prefix}/share/doc/libpkgsource/
```

Committed generated manual pages install under the ordinary man hierarchy.
Normal builds therefore do not require Pandoc or Doxygen.

## HTML documentation

HTML is an explicit versioned artifact:

```sh
meson setup build-docs \
  -Dhtml_docs=enabled \
  -Dman_pages=enabled
meson compile -C build-docs html-docs
```

Installation places the generated tree under:

```text
${prefix}/share/htmldocs/libpkgsource/3.0.1/
```

The publishing site may copy that tree unchanged. It does not become another
documentation authority.

GPL-3.0-or-later. See `COPYING` and `COPYRIGHT`.
