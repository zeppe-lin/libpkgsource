// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/recipe_fixture.h"

namespace test_support::recipe_fixture {

void test_snapshot_reconstruction_authentication()
{
  const profile_catalog catalog = profiles();
  const source_snapshot valid =
      seal_source(source_origin("recipe.yml"), declaration(), catalog);

  expect(error_code::invalid_identity, [&] {
    (void)source_snapshot(
        valid.origin(),
        valid.recipe(),
        source_snapshot_identity::from_sha256(std::string(64, '0')));
  });

  const source_snapshot another_origin(
      source_origin("another-recipe.yml"), valid.recipe(), valid.identity());
  assert(another_origin.identity() == valid.identity());
  assert(another_origin.origin().document() == "another-recipe.yml");
}

} // namespace test_support::recipe_fixture

int main()
{
  test_support::recipe_fixture::test_snapshot_reconstruction_authentication();
}
