<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Testing

The test suite is offline and uses the bundled `tests/corpus` directory plus
per-test temporary trees and private workers.

Coverage includes:

* model and package-identity invariants;
* metadata header parsing and dependency compatibility mapping;
* worker NUL framing and exact record cardinality;
* explicit environment use, ambient-environment isolation, optional worker
  identity, and unsafe-variable rejection;
* source expansion from the captured root even when top-level Pkgfile code
  changes directory;
* worker-descendant process-group cleanup;
* `.md5sum` closure, duplicate, malformed, missing, and unrelated entries;
* `.nostrip` POSIX ERE normalization;
* lifecycle and README discovery;
* `.footprint` and `.32bit` declaration capture;
* complete-directory and internal-symlink capture;
* digest stability, directory-mode sensitivity, and snapshot-owned file
  lifetime;
* original-tree and captured-tree mutation detection;
* path and symbolic-link escape rejection;
* unsupported special-object rejection;
* deterministic reference-tool output;
* independent public-header compilation;
* consumer linkage in the active configuration.

Run the normal suite:

```sh
meson setup build \
  -Ddefault_library=shared \
  -Dlink_mode=shared
meson compile -C build
meson test -C build --print-errorlogs
```

Run both supported linkage configurations:

```sh
tests/run-linkage-matrix.sh .
```

The CI workflow executes the same shared/static matrix.  `default_library=both`
is intentionally not a test configuration because it is an invalid project
configuration.

No test downloads source archives or accesses a package collection over the
network.
