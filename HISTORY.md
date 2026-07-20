<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# History

## 0.1.0

Initial repository and first package-management architecture implementation.

* established the immutable package-source model and backend contract;
* implemented the complete `pkgfile/0` source-directory protocol;
* replaced shell-assignment parsing with a private evaluated worker record;
* captured complete package directories into lifetime-owned private snapshots;
* normalized metadata, compatibility dependencies, sources, MD5 declarations,
  recipe descriptor, lifecycle actions, README resources, strip exclusions,
  footprint declaration, and `.32bit` mode;
* added deterministic snapshot fingerprints and stable reference output;
* added offline corpus, race and escape tests, shared/static build matrix,
  contract documents, and scdoc manual pages.
