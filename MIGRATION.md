<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Migration notes

No existing consumer is changed by version 0.1.0.  This repository is an
independent implementation and does not integrate with pkgman or libpkgbuild.

## Future pkgman migration

The eventual pkgman adapter should replace `Package::load()` and direct
filename probing with one `source_snapshot` per selected package source.
Collection walking, precedence, duplicate reporting, and shadowing remain
pkgman responsibilities.  pkgman should consume typed metadata, dependencies,
README resources, and lifecycle declarations from the snapshot and must not
reparse the Pkgfile.

The adapter must decide snapshot caching and invalidation at the collection
layer.  The library fingerprint is suitable as a captured-content identity but
does not decide collection freshness policy.

## Future libpkgbuild migration

The eventual build adapter should accept the snapshot's source inputs, declared
MD5 values, recipe descriptor, strip exclusions, footprint declaration, and
architecture mode.  Download, content verification, extraction, recipe
execution, staged-image construction, normalization, and footprint comparison
remain build-layer work.

The build adapter must execute from the captured root and bind the stored
identity to execution.  It should not independently source the mutable original
Pkgfile or rebuild source declarations from text.

`PkgfileDefinitionLoader` and its worker can be retired only after that adapter
covers the existing build engine's configuration and archive-policy inputs.

## Compatibility cautions

Version 0.1.0 intentionally narrows or formalizes behavior that older heuristic
readers accepted ambiguously:

* metadata labels are exact rather than prefix abbreviations;
* duplicate metadata and dependency names are errors;
* checksum manifests must exactly close over source local names;
* binary-mode MD5 lines are unsupported;
* sidecars must be regular files;
* unsafe and external symlinks are rejected;
* filesystem races are errors rather than silently mixed revisions;
* no pkgmk configuration file is sourced;
* the legacy `local-name::remote-locator` form is normalized explicitly,
  with `.md5sum` bound to `local-name`;
* `.footprint` is captured but not parsed.

Before replacing a production reader, run the reference client over each
collection and classify failures as source defects, required compatibility
extensions, or intentionally rejected ambiguity.  Do not weaken the immutable
snapshot invariant to accommodate mutable collection paths.
