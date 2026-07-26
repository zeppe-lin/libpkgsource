<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# libpkgsource

`libpkgsource` inspects one package-source directory and returns an immutable,
normalized description of what that source declares.

Version 0.2.0 implements one format only: `pkgfile/0`.  In this library,
`pkgfile/0` means the complete legacy source-directory protocol, not only the
`Pkgfile` shell program.  The inspected unit includes metadata comments,
identity and source declarations, the `build()` entry point, `.md5sum`,
`.footprint`, `.nostrip`, `.32bit`, README resources, lifecycle programs, and
all other files needed to preserve the executable directory context.

The library does not scan collections, choose repository precedence, resolve
dependencies, download or verify distfiles, execute recipes or lifecycle
actions, create package images, inspect package archives, or touch installed
state.  It has no dependency on pkgman, libpkgbuild, libpkgimage, or
libpkgstate.

## Contract

Inspection proceeds in four stages:

1. copy the complete source directory into a private snapshot while rejecting
   unsafe paths, escaping links, races, and unsupported filesystem objects;
2. evaluate the captured `Pkgfile` in a private POSIX-shell worker with an
   explicit environment, working directory, umask, and optional execution
   identity;
3. normalize the shell result and all legacy sidecars into typed immutable
   objects, enforcing checksum closure and source-directory invariants;
4. verify that neither the original source nor the captured tree changed, then
   seal the snapshot read-only for the lifetime of the returned object graph.

The returned `source_snapshot`, every `captured_file`, every lifecycle action,
and every local source input share ownership of the private tree.  A captured
path therefore cannot outlive the contents to which it refers.

See [DESIGN.md](DESIGN.md), [PKGFILE-0.md](PKGFILE-0.md), and
[SNAPSHOTS.md](SNAPSHOTS.md) for the normative contracts.

## Build

Shared and static libraries are separate Meson configurations.  `both` is
rejected.

```sh
meson setup build-shared \
  -Ddefault_library=shared \
  -Dlink_mode=shared
meson compile -C build-shared
meson test -C build-shared --print-errorlogs

meson setup build-static \
  -Ddefault_library=static \
  -Dlink_mode=static
meson compile -C build-static
meson test -C build-static --print-errorlogs
```

Tests are enabled by default.  Manual pages are built when `scdoc` is
available, or can be required with `-Dman_pages=enabled`.

The reference client is built when tests are enabled or when
`-Dreference_tools=enabled` is selected.  It is never installed by this
release:

```sh
build-shared/tools/pkgsource-inspect \
  --worker build-shared/pkgsource-pkgfile-worker \
  /path/to/collection/package
```

Its output is a stable diagnostic JSON representation.  It is not a command
API and is not a substitute for linking against the library.

## Security position

A Pkgfile is executable shell.  The worker removes ambient environment and
loader injection, separates diagnostics from framed output, applies explicit
process state, and rejects malformed records.  It is not a syscall, network,
or mount sandbox.  Inspect only source trees appropriate for the worker's
execution identity; callers running as root should normally request an
unprivileged identity.

## License

GPL-3.0-or-later.  See `COPYING` and `COPYRIGHT`.

## Optional planner adapter

`libpkgsource-plan` is an optional composition library, enabled with
`-Dplanner_adapter=enabled`, that projects one sealed source snapshot into a
`libpkgplan` candidate package fact.  It keeps the source
snapshot alive, preserves exact removal lifecycle bytes, and computes
source-owned release and candidate-control identities.  The core
`libpkgsource` library does not depend on `libpkgplan`.
