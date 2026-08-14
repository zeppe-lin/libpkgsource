# Contributing

## Boundary first

Changes must preserve the source-owner boundary. The core accepts
parser-neutral declarations and emits sealed authority. YAML parsing,
collection acquisition, resolution, fetching, execution, planner projection,
state publication, and evidence-store policy do not belong here.

Before changing a public value or invariant, identify:

1. which owner field changes;
2. whether semantic identity material changes;
3. whether durable record bytes change;
4. whether the core ABI or codec ABI changes;
5. which downstream owner consumes the result.

Do not preserve an experimental field through aliases or compatibility branches
without a published consumer that requires it.

## Code and API changes

Follow `docs/code-style.md`. Every public declaration must be self-contained
and fully documented for strict Doxygen. Public symbols require explicit export
annotations and review against the corresponding manifest under `abi/`.

Comments explain invariants, framing, ownership transfer, or failure
translation. They do not paraphrase the next statement. Control-flow bodies
always use braces.

## Identity and record changes

Identity domains, field participation, field order, tag values, and
normalization are protocols. A provider replacement that preserves SHA-256
bytes is an implementation change; changing the algorithm or framing is not.

Codec changes require bounded-decoder tests, identity-substitution tests,
canonical re-encoding tests, fixed vectors, and an explicit schema decision in
`docs/protocols/source-records-v1.md`.

## Tests and documentation

Add focused tests under the authority that can fail. Do not grow a monolithic
regression executable. Update the public headers, manuals, architecture,
testing policy, history, and generated artifacts in the same reviewable series.

Canonical manuals live under `docs/man/`. Regenerate committed roff with:

```sh
tools/update-man-pages.sh --write
```

Before submission, run the complete Meson suite, strict Doxygen, manual
regeneration and lint, formatting, `git diff --check`, and `git fsck --full`.
