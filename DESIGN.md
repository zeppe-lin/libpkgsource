<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Design

## Problem statement

The legacy package-source meaning was previously split across three places:

* pkgman heuristically read selected `Pkgfile` lines and inferred README and
  lifecycle presence from filenames;
* libpkgbuild evaluated part of the Pkgfile through a worker and normalized
  only the inputs needed by its build engine;
* callers independently treated sidecar files as implicit protocol.

That arrangement made collection policy, source semantics, build execution,
and mutable filesystem paths bleed into each other.  `libpkgsource` establishes
one contract: inspect exactly one source directory, capture it, normalize it,
and stop.

## Boundary

The public abstraction is `source_backend`.  A backend probes a
`source_location` and performs an `inspect_request`.  The first backend is
`pkgfile_backend`, advertising `source_format::pkgfile_v0`.

The result is a `source_snapshot`.  It contains:

* origin and source format;
* a deterministic SHA-256 fingerprint of the complete captured tree;
* a `build_description` with identity, descriptive metadata, dependencies,
  source inputs and declared MD5 digests, recipe descriptor, lifecycle
  declarations, README resources, strip exclusions, footprint declaration,
  and legacy architecture mode;
* shared ownership of the private captured tree.

The library intentionally has no collection, graph, download, extraction,
build, image, archive, installation, or installed-state abstraction.

## Immutability

Public model classes expose construction-time values through const accessors
and have no mutators.  Files are represented by `captured_file`, which owns a
shared reference to private snapshot state and records the relative path,
original mode, size, and content SHA-256.

The native snapshot path is exposed for future execution adapters, but is
sealed to owner-read/execute permissions before return.  This is an OS-level
accident barrier, not an adversarial guarantee against the owning process,
which can change permissions.  Any caller that deliberately mutates the native
path violates the object contract.

## Snapshot transaction

The capture transaction is described fully in `SNAPSHOTS.md`.  Its invariants
are:

* the original tree contains directories, regular files, and safe internal
  symbolic links only;
* the complete directory is copied without following source symlinks;
* regular files are opened with `O_NOFOLLOW` and checked before and after read;
* the original manifest is rescanned after copy and after worker evaluation;
* the captured manifest is rescanned after worker evaluation and after all
  protocol parsing;
* any difference in entry set, type, path, mode, size, identity, timestamps,
  link target, or content digest aborts inspection;
* the returned fingerprint is independent of snapshot path, uid, gid, and
  timestamps, but includes every relative path, entry type, permission mode,
  regular-file content, and symbolic-link target.

## Pkgfile evaluation

C++ does not parse shell assignments.  The backend executes a private worker
using `/bin/sh` and `execve()` with a newly constructed environment.  The
worker sources the captured Pkgfile, checks the required variables and
`build()` function, performs legacy unquoted `source` word expansion, and
writes a NUL-framed record.  Incidental Pkgfile standard output is redirected
to the worker diagnostic channel.

The C++ side validates the protocol marker, exact field count, source count,
identity values, entry-point marker, and directory/name invariant before
constructing public objects.  Source normalization recognizes the legacy
`local-name::remote-locator` form; the explicit local name, rather than the
locator basename, owns the checksum identity.

No configuration file is sourced in this vertical slice.  Build-engine policy
and pkgmk configuration do not belong to source inspection.

## Metadata compatibility

The initial dependency model maps the legacy `Depends on:` comment to
`dependency_scope::build_and_run`.  This is explicitly a `pkgfile/0`
compatibility scope, not the final package dependency taxonomy.

Metadata comments are parsed only from the initial blank/comment header of the
Pkgfile.  Recognized labels are exact, case-insensitive names:
`Description`, `URL`, `Packager`, `Maintainer`, and `Depends on`.  Unknown
comment fields remain ordinary comments.  Duplicate, empty, or malformed
recognized fields are rejected.

## Footprint choice

Version 0.1.0 does not parse `.footprint` entries.  It exposes a typed
`footprint_declaration` whose format is `pkgfile_footprint_v0` and whose file is
bound to the snapshot.

This is the narrower defensible contract.  A footprint is a build expectation
whose detailed grammar and comparison policy belong at the source/build
boundary.  Parsing it here before that adapter contract exists would either
freeze pkgmk output syntax prematurely or smuggle package-image semantics into
a source-inspection library.

## Error model

All contract failures throw `pkgsource::error` with an `error_code`.  The code
separates request errors, unsafe trees, source mutation, worker failure,
malformed worker records, invalid Pkgfile semantics, metadata errors, checksum
errors, sidecar errors, and filesystem failures.  Error strings are diagnostic;
callers should branch on the code.

## Planner projection adapter

The core library remains independent of package planning.  An optional
`libpkgsource-plan` adapter may translate one immutable `source_snapshot` into
planner-owned incoming control without reopening the live collection.

The adapter owns exactly this boundary:

```text
source snapshot
  package identity
  dependency declarations
  removal lifecycle programs
  build architecture
        |
        v
libpkgplan candidate package fact
```

It does not inspect package archives, issue artifact identities, resolve
runtime dependency closure, select path policy, observe the target filesystem,
or create an install or upgrade request.

The adapter must retain the source snapshot beside the projected planner fact.
This prevents callers from pairing candidate control with a different or later
source observation.  Runtime dependencies are projected from declarations whose
scope includes runtime use.  Only pre-remove and post-remove lifecycle programs
enter candidate control; installation lifecycle programs remain build/application
inputs.  Lifecycle material is read from the sealed captured tree and retained
as exact bytes with media type `text/x-shellscript`.

The adapter issues domain-separated versioned SHA-256 identities for package
release and normalized candidate control.  Candidate-control identity is computed
from the normalized planner projection, not from unrelated README, recipe, or
source-input bytes.  The complete source snapshot fingerprint remains available
as separate provenance and is never relabelled as a planner identity.

Build-system worker location contract
-------------------------------------

The private Pkgfile worker is owned by libpkgsource.  Its pathname is a runtime
component location, not source-model data and not a consumer-owned convention.
The `libpkgsource` Meson dependency therefore publishes one
`pkgfile_worker` variable through both dependency forms:

. an internal dependency returns the configured build-tree worker path; and
. an installed pkg-config dependency returns the installed libexec path.

A consumer that must instantiate `pkgfile_backend` explicitly for a build-tree
test obtains the path with `dependency.get_variable()`.  It does not reach into
the libpkgsource source tree, guess a sibling build directory, or substitute a
different package worker protocol.  Ordinary installed consumers continue to
use the default constructor and the library's compiled installed path.
