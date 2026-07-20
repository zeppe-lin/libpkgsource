<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Source snapshots

## Why a copy is required

A normalized source description that retains paths into a live collection is
not immutable.  The collection can be updated between inspection and build,
causing identity, inputs, sidecars, and executed code to come from different
revisions.  `source_snapshot` therefore owns a private copy and all file
objects reference that copy.

For `pkgfile/0`, the complete directory is captured.  An executable Pkgfile can
read undeclared siblings, test file modes, follow internal links, and expand
pathnames.  Capturing only the files recognized by C++ would create a new and
incompatible language.

## Capture algorithm

1. Resolve the source root without accepting a non-directory.
2. Recursively enumerate with `lstat()`, sorted by raw directory-entry name.
3. Reject unsupported objects and validate every symbolic link by complete
   canonical resolution against the source root.
4. Hash every regular file through an `O_NOFOLLOW` descriptor and compare its
   identity, size, modification time, and change time before and after read.
5. Create a private `0700` directory outside the source root.
6. Recreate directories privately, copy regular files through `O_NOFOLLOW`
   descriptors, recreate safe links, and then restore source directory modes.
   The caller's ambient umask therefore cannot change the captured protocol.
7. Rescan the original tree and require exact equality with the first manifest.
8. Scan and hash the captured tree to establish its pre-evaluation manifest.
9. Evaluate and parse the Pkgfile and sidecars.
10. Rescan both original and captured trees.  Any difference aborts inspection.
11. Seal captured regular files to owner-read plus their original executable
    bit, and directories to owner-read/execute.
12. Return shared ownership of the tree and its deterministic fingerprint.

## Symbolic links

Links are copied as links.  Their target text is included in the fingerprint.
A link is accepted only when it is relative, non-dangling, non-cyclic, and its
complete resolution remains inside the source root.  The same topology is then
present in the captured root.

Hard-linked regular files are copied as independent regular files.  This avoids
retaining an inode relationship to content outside the private tree.  The
protocol fingerprint is path/content based and does not encode hardlink groups.

## Fingerprint

The fingerprint is SHA-256 over a versioned canonical record stream sorted by
relative path.  Records include:

* entry type;
* relative path;
* permission bits;
* regular-file size and SHA-256;
* symbolic-link target.

Directory times, file times, inode numbers, device numbers, uid, gid, and the
random private root are excluded.  Two captures of unchanged contents produce
the same fingerprint.

## Lifetime

`source_snapshot` and `captured_file` share an internal state object.  The tree
is removed only after the final owner is destroyed.  A recipe descriptor or
lifecycle declaration can therefore be retained independently without turning
its path into a dangling reference.

The public `native_root()` and `captured_file::native_path()` accessors exist
for future adapters that need a filesystem working directory.  Callers MUST
regard those paths as read-only.  Deliberately changing permissions or contents
breaks the contract and is outside library guarantees.

## Failure cleanup

A failed capture owns its temporary root through the same state object.  Cleanup
makes protected directories writable to their owner and removes the tree
recursively.  Cleanup errors in destructors are ignored; the inspection error
that caused rollback remains primary.
