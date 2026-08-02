<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Design

## Authority reset

`libpkgsource` is the authority for the native Zeppe-Lin package-source model.
It does not preserve the CRUX package-source protocol and does not expose a
compatibility interpretation of `Pkgfile`, its comments, or its sidecars.
Compatibility belongs in explicit migration programs outside this library.

The reset is intentionally incompatible.  Native source truth is defined from
package semantics needed by later architecture stages, not from the subset of
meaning recoverable from a historical source directory.

## Authority boundary

A syntax reader may parse `recipe.yml` or another future input syntax into
unsealed declarations.  Parsed declarations are not authority.  They may retain
spelling, order, and source positions for diagnostics, but no later stage may
consume them as package truth.

The semantic boundary is sealing:

```text
input syntax + sealed profile catalog
                |
                v
        native source sealer
                |
                v
       immutable source snapshot
```

The sealed snapshot owns normalized package release, metadata, source inputs,
build program, optional check program, requirements, lifecycle programs,
lifecycle requirements, architecture requirements, selected build profiles,
the exact profile closure used for expansion, and domain-separated identities.

The library does not scan collections, select precedence, resolve available
packages, download or verify source objects, execute programs, construct package
images, inspect archives, install packages, or read installed state.

## YAML syntax adapter

`libpkgsource-yaml` is a separate syntax-boundary repository.  It depends on libyaml and
translates raw `profiles.yml/1`, `recipe.yml/1`, and `recipe.yml/2` bytes into
the same parser-neutral declarations accepted by the core.  The core library has no YAML
dependency, and parsed documents never become an alternative authority model.

The adapter is deliberately strict: one document, exact field sets, duplicate
key rejection, explicit package/profile subject mappings, and no anchors,
aliases, merge keys, custom tags, or directives.  It retains document, schema
path, line, and column for diagnostics while leaving those values outside
semantic identities.

Profile syntax and recipe syntax remain separate.  Profiles are parsed and
sealed before recipes so a recipe can only select already authoritative profile
values.  Collection acquisition, document discovery, and cross-collection
profile policy belong to `libpkgcatalog` acquisition tooling, not to this
adapter.

## Requirement model

Requirement scope and requirement subject are independent typed domains.

Scopes are:

* build;
* run;
* lifecycle, bound to exactly one lifecycle action; and
* check, retained as a first-class domain for later check execution.

`recipe.yml/1` reserves check requirements without executable check authority.
`recipe.yml/2` may bind them to one exact optional check program.  The source
sealer rejects version-two check requirements when that program is absent.  A
check program without additional requirements is valid.

Subjects are:

* exact package references; and
* exact profile references.

There is no native `build_and_run` scope.  A package required in both domains is
represented by two declarations and remains distinguishable after sealing.

## Profiles

Profiles are authoritative package-set values, not parser macros or aliases.
A profile has a normalized name, a retained declaration site, direct package or
profile members, a deterministic transitive expansion, and a semantic identity.

The profile catalog is sealed before recipes are sealed.  Sealing rejects
unknown references, duplicate definitions, duplicate direct members, and every
cycle.  Definitions and expansions are normalized independently of caller
insertion order.  A nested profile identity contributes to its parent's
identity, so changing a nested definition changes every enclosing semantic
value.

A source snapshot retains the selected build-profile roots and the complete
transitive profile closure used to expand its requirements.  Later stages do
not need to reopen a mutable profile database to understand the snapshot.

## Identity domains

Package names, profile names, and architecture names use strict canonical ASCII
identities.  Non-canonical spellings are rejected rather than treated as
aliases.

Semantic SHA-256 identities are domain-separated and versioned.  At minimum the
library distinguishes profile definition identity, package release identity,
normalized recipe identity, and source snapshot identity.  Declaration
provenance does not change semantic identity, but it is retained for diagnostics
and audit.

Version-one recipe identity encoding remains unchanged.  A recipe with an exact
check program uses the version-two recipe identity domain and includes the
program language and exact bytes.  Source syntax remains diagnostic provenance;
a version-two document with no added check semantics may therefore share the
same semantic identities as an equivalent version-one declaration.

## Planner projection

`libpkgsource-plan` is a separate composition-boundary repository.  It may
project only facts already sealed by `libpkgsource` into `libpkgplan` values.
It must not resolve profiles, reinterpret syntax, reopen a collection, or infer
runtime semantics from build declarations.

The native projection sends exact run requirements, durable removal lifecycle
programs, and normalized target-architecture requirements to `libpkgplan`.
Build requirements, check requirements, lifecycle requirements, selected build
profiles, build architecture requirements, source inputs, build programs, and
check programs remain upstream execution or resolution inputs.

The current `libpkgplan` candidate-control API already has the required runtime
dependency, removal lifecycle, and target-profile value domains.  No planner API
change is required for this reset.  Future dependency resolution in `pkgctl`
will consume the remaining source requirement domains before invoking package
planning.

## Durable representation boundary

A sealed profile catalog and source snapshot are long-lived semantic authority,
not merely products of one parser process.  Their durable representation is
owned by `libpkgsource`.

The profile-catalog record stores declaration-level profile definitions and
expected profile identities.  Decode invokes `profile_catalog::seal()` and
refuses any identity or canonical-byte disagreement.

The source-snapshot record stores diagnostic origin and syntax, the exact recipe
declaration fields, and a nested canonical encoding of the snapshot's retained
profile closure.  Original requirement declarations are reconstructed from the
sealed requirement origins: direct requirements retain an empty expansion path,
while profile requirements retain the selected root as the first expansion
step.  Decode invokes `seal_source()` and refuses any recipe or source identity
change.

The records deliberately do not embed YAML documents, collection precedence,
fetch material, execution results, or downstream planning state.  Reopening a
source snapshot is semantic reconstruction through the owner sealers, not input
reacquisition and not deserialization of private object layout.
