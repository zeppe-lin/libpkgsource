<!-- SPDX-FileCopyrightText: 2026 Alexandr Savca -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Contributing

Changes must preserve the source-owner boundary. The core accepts parser-neutral
declarations and emits sealed authority. YAML parsing, collection acquisition,
resolution, execution, planner projection, state publication, and store policy do
not belong in this repository.

Before changing a public value or invariant, identify:

1. which owner field changes;
2. whether source semantic identity must change;
3. whether durable record bytes must change;
4. whether the core ABI or codec ABI changes; and
5. which downstream owner consumes the result.

Do not preserve an experimental field through aliases or compatibility branches
without an actual published consumer that requires it.

Core changes require model tests, deterministic identity tests, negative
invariant tests, public-header compilation, and shared/static qualification.
Codec changes additionally require bounded-decoder tests, stored-identity
substitution tests, canonical re-encoding tests, golden vectors, and an explicit
schema-version decision in `SOURCE-RECORDS-1.md`.

Keep implementation, tests, documentation, CI, and release preparation in
reviewable commits. Run `git diff --check` and the complete Meson suite before
submission.
