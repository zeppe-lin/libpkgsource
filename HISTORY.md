<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# History

## 2.0.0

Explicit native check-program authority.

* add an optional exact check program to parser-neutral recipe declarations and
  sealed recipe authority without adding execution behavior;
* preserve the published `recipe.yml/1` protocol and its semantic identity
  vectors unchanged;
* define strict `recipe.yml/2` input with one optional top-level check program;
* require version-two check requirements to be closed by a check program while
  allowing a check program with no additional check requirements;
* include exact check-program language and bytes in version-two recipe and
  source-snapshot identities;
* keep check execution environment, result evidence, transaction binding, and
  scheduling outside `libpkgsource`; and
* keep check programs outside the durable planner candidate projection.

## 1.1.0

Native YAML syntax frontend.

* add optional `libpkgsource-yaml.so.1` without adding a YAML dependency to the
  semantic core;
* define and parse strict `profiles.yml/1` documents into parser-neutral profile
  declarations;
* parse strict `recipe.yml/1` documents into the existing native recipe
  declarations;
* retain structured document, schema path, line, and column diagnostics while
  excluding syntax provenance from semantic identity;
* reject duplicate and unknown keys, multiple documents, directives, anchors,
  aliases, merge keys, unsupported tags, scalar requirement shorthand, and
  schema/type drift;
* provide convenience functions that parse and invoke the existing profile and
  source sealers without creating another authority model; and
* keep collection discovery, document acquisition, profile aggregation, and
  precedence outside the parser and core libraries.

## 1.0.0

Native package-source authority reset.

* remove the Pkgfile/0 backend, evaluated shell worker, metadata comments,
  sidecars, MD5 declarations, captured legacy source tree, `.32bit` marker, and
  all CRUX compatibility API;
* replace `dependency_scope::build_and_run` with exact build, run, check, and
  action-bound lifecycle scopes;
* add exact package, profile, and architecture identity domains with strict
  canonical spelling;
* add authoritative sealed profile definitions with deterministic nested
  expansion, cycle and duplicate rejection, semantic identities, and retained
  declaration provenance;
* add the parser-neutral native recipe declaration and immutable normalized
  recipe/source snapshot authority;
* distinguish package release, metadata, source inputs, build program,
  requirement domains, lifecycle programs and requirements, architecture
  requirements, selected build profiles, and profile closure;
* define the exact `recipe.yml/1` input contract without embedding a YAML parser
  or treating syntax as authority;
* rebase `libpkgsource-plan` on exact run requirements, durable removal
  lifecycle programs, and target architecture requirements without changing
  `libpkgplan` 0.2; and
* begin SONAME 1 for both `libpkgsource` and `libpkgsource-plan`.

Migration from Pkgfile/0 and the historical package database remains outside the
libraries and will be implemented by separate explicit import tools.

## 0.2.1

* publish the private Pkgfile worker location through internal Meson and
  installed pkg-config dependency metadata;
* qualify the worker variable in build-tree and isolated installed shared/static
  configurations; and
* document the worker pathname as a dependency-owned runtime location rather
  than a consumer convention.

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

## 0.1.0

Initial repository and first package-management architecture implementation.

* established the immutable package-source model and backend contract;
* implemented the complete `pkgfile/0` source-directory protocol;
* replaced shell-assignment parsing with a private evaluated worker record;
* captured complete package directories into lifetime-owned private snapshots;
* normalized metadata, compatibility dependencies, sources, MD5 declarations,
  recipe descriptor, lifecycle actions, README resources, strip exclusions,
  footprint declaration, and `.32bit` mode; and
* added deterministic snapshot fingerprints, corpus tests, build qualification,
  contract documents, and scdoc manual pages.
