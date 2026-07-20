<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# pkgfile/0 package-source directory protocol

This document is the first formal specification of the complete legacy
Pkgfile package-source directory protocol as implemented by libpkgsource
0.1.0.

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, and **MAY** are
to be interpreted as normative requirements.

## 1. Protocol unit

A `pkgfile/0` source is one filesystem directory.  Its basename is the expected
package name.  `Pkgfile` is REQUIRED.  The directory MAY also contain declared
local sources, build auxiliaries, checksum and build sidecars, README
resources, and lifecycle programs.

Because `Pkgfile` is executable shell and can inspect arbitrary siblings, the
protocol unit is the complete directory, including undeclared siblings.  A
consumer MUST NOT treat only the named sidecars as the source of truth.

## 2. Filesystem domain

Directories, regular files, and symbolic links whose complete resolution stays
inside the protocol root are supported.  Absolute, dangling, cyclic, or
escaping symbolic links are invalid.  FIFOs, sockets, devices, and other
special objects are invalid.

Inspection MUST NOT silently follow a source-tree link outside the root.
Mutation detected while reading or evaluating the directory is invalid.

## 3. Pkgfile

`Pkgfile` MUST be a regular file containing executable POSIX-shell code.  It is
sourced by a fresh non-interactive `/bin/sh` worker whose current directory is
the captured package-source root.

After evaluation, the following shell variables are REQUIRED and MUST be
non-empty:

```text
name
version
release
```

`name` MUST equal the package-source directory basename.  The three identity
components MUST contain no slash, ASCII whitespace, or control character.
Their exact shell-evaluated strings are preserved.

The variable `source` is optional.  An unset or empty value declares no build
inputs.

A shell function named `build` is REQUIRED.  libpkgsource records the typed
entry point but never calls it.

## 4. Evaluation environment

The inspector constructs the worker environment from an explicit caller map
plus fixed values for `PATH`, `LANG`, `LC_ALL`, `TMPDIR`, `HOME`, `USER`, and
`LOGNAME`.  Ambient process environment is not inherited.

Loader and shell-startup injection variables, including `LD_*`, `BASH_ENV`,
`ENV`, `CDPATH`, `IFS`, `SHELLOPTS`, `PYTHONPATH`, `PERL5LIB`, `RUBYOPT`,
`GCONV_PATH`, `LOCPATH`, and `NLSPATH`, are rejected when supplied by the
caller.  `PKGSOURCE_*` names are reserved.

The worker has an explicit current directory and umask.  The caller MAY select
an execution uid/gid.  When the inspector has sufficient privilege, the
private tree is temporarily owned by that identity and restored before return.
The worker is placed in a private process group; descendants remaining after
the evaluation shell exits are terminated before inspection continues.

This process boundary is not a syscall or network sandbox.  A Pkgfile that
deliberately escapes its process group is outside the containment guarantee.

## 5. Metadata comment header

Metadata is read only from the initial header: blank lines and comment lines
before the first non-comment shell line.

A metadata line has this form:

```text
# Label: value
```

Horizontal whitespace around the label, colon, and value is ignored.  Labels
are matched case-insensitively.  The recognized labels are:

```text
Description
URL
Packager
Maintainer
Depends on
```

Each recognized label MAY occur at most once and its value MUST be non-empty.
A recognized label without a colon is malformed.  Unknown labels and ordinary
comments have no semantic effect.

`Depends on:` is split on ASCII shell-style whitespace.  Each token becomes a
`dependency` with scope `build_and_run`.  Duplicate dependency names are
invalid.  `build_and_run` is a compatibility mapping for the legacy field, not
a final dependency model.

## 6. Source declaration expansion

After the Pkgfile has been evaluated, the worker restores its current
directory to the captured package-source root, then applies unquoted shell
expansion equivalent to:

```sh
set -- $source
```

Field splitting and pathname expansion are therefore part of `pkgfile/0`.
Each resulting word is one source declaration, in shell-produced order.

A remote declaration has either of these forms:

```text
remote-locator
local-name::remote-locator
```

`remote-locator` MUST contain a URI scheme separator (`://`).  In the second
form, the first `::` before that scheme separator is the legacy explicit local
name delimiter.  `local-name` MUST be a safe basename.  It is the checksum and
distfile identity; the suffix after `::` is the normalized remote locator.
For example:

```text
run-one-1.18.tar.gz::https://example.invalid/archive/1.18.tar.gz
```

normalizes to local name `run-one-1.18.tar.gz` and locator
`https://example.invalid/archive/1.18.tar.gz`.  Without an explicit local name,
the local name is the locator's final path component after removing a query
and fragment.

Declarations without a remote locator are recipe-local inputs and MUST be safe
normalized relative paths.  Their local name is the basename.

Every local name MUST be non-empty and unique across the declaration list.
Each recipe-local input MUST name a captured regular file.  libpkgsource does
not download remote inputs and does not verify their contents.

## 7. .md5sum

When at least one source is declared, `.md5sum` is REQUIRED and MUST be a
regular text file.  Blank lines and lines whose first non-whitespace character
is `#` are ignored.  Every remaining line has:

```text
32-lower-or-upper-hex-digits  basename
```

The digest is normalized to lowercase and typed as MD5.  The filename MUST be
a single safe basename.  GNU binary-mode `*filename` notation is unsupported.

The manifest MUST form an exact one-to-one closure over source local names:

* every declared source has one entry;
* duplicate entries are invalid;
* missing entries are invalid;
* unrelated entries are invalid.

When no sources are declared, `.md5sum` MAY be absent or contain comments and
blank lines only.  libpkgsource never invents a missing checksum and never
hashes or verifies a downloaded distfile.

## 8. .nostrip

`.nostrip` is optional.  When present, it MUST be a regular text file.  Each
line, with only a terminal CR removed, is one POSIX extended regular expression
and is preserved in order.  Invalid expressions are rejected.  Empty lines are
valid empty regular expressions and are not silently discarded.

The result is a list of typed strip exclusions.  The library does not perform
stripping.

## 9. .footprint

`.footprint` is optional and MUST be a regular file when present.  It is
normalized as `footprint_declaration(pkgfile_footprint_v0)` bound to the
snapshot.  Version 0.1.0 deliberately does not parse its line grammar.

The declaration is a build expectation.  It is not a package image and not an
installed ownership manifest.

## 10. .32bit

`.32bit` is an optional regular marker file.  Its presence selects
`build_architecture::legacy_32bit`; absence selects
`build_architecture::native`.  File contents have no additional semantics in
this version, but are included in the snapshot fingerprint.

## 11. README resources

`README` and `README.md` are independently optional regular files.  `README`
normalizes to a plain-text README resource; `README.md` normalizes to a Markdown
README resource.  When both exist, both are declared in that order.

No content rendering or preference policy is imposed by the library.

## 12. Lifecycle actions

The following optional regular files map to typed phases:

```text
pre-install  -> pre_install
post-install -> post_install
pre-remove   -> pre_remove
post-remove  -> post_remove
```

Each declaration carries a captured program, original mode, size, and SHA-256
content digest.  Executable permission is descriptive, not an execution gate.
libpkgsource never executes a lifecycle action.

## 13. Snapshot fingerprint

The complete captured directory is fingerprinted with SHA-256 over a canonical
ordered record stream.  The stream includes every relative path, entry type,
permission mode, regular-file size and content digest, and symbolic-link
target.  It excludes snapshot location, uid, gid, and timestamps.

The fingerprint identifies captured protocol contents.  The evaluated model is
stored immutably alongside it; environment-sensitive shell commands can still
produce different values from identical contents under different explicit
inspection conditions.

## 14. Required rejections

The backend rejects at least:

* missing or non-regular `Pkgfile`;
* empty or invalid name, version, or release;
* directory basename/name mismatch;
* missing `build()` function;
* malformed or inconsistent worker framing;
* malformed, duplicate, or empty recognized metadata fields;
* malformed or duplicate checksum entries;
* missing checksums and unrelated checksum entries;
* duplicate source local names;
* unsafe local paths;
* escaping, dangling, or cyclic symbolic links;
* original or captured-tree mutation during inspection;
* unsupported special filesystem objects;
* non-regular protocol sidecars.
