<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# History

## 0.2.0

* normalize the legacy `local-name::remote-locator` source form and bind its
  checksum declaration to the explicit local name;
* add the optional `libpkgsource-plan` adapter without contaminating the core
  source-inspection dependency closure;
* project runtime dependencies, exact removal lifecycle bytes, and package
  build architecture into complete planner candidate control;
* issue domain-separated package-release and normalized candidate-control
  identities while retaining the source snapshot as separate provenance; and
* qualify the adapter against libpkgplan 0.2.0 and libpkgimage 0.3.0 in shared
  and static configurations.

The core library remains at SONAME 0.  The new planner adapter begins at
SONAME 0 as its first released ABI.

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
