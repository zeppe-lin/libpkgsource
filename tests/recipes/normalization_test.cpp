// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/recipe_fixture.h"

namespace test_support::recipe_fixture {

void test_identity_normalization()
{
  const profile_catalog catalog = profiles();
  source_snapshot first =
      seal_source(source_origin("a.yml"), declaration(false), catalog);
  source_snapshot reordered =
      seal_source(source_origin("b.yml"), declaration(true), catalog);
  source_snapshot changed = seal_source(
      source_origin("a.yml"), declaration(false, "echo changed\n"), catalog);
  assert(first.identity() == reordered.identity());
  assert(first.identity() != changed.identity());
}

} // namespace test_support::recipe_fixture

int main()
{
  test_support::recipe_fixture::test_identity_normalization();
}
