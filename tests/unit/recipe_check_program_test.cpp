// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/recipe_fixture.h"

namespace test_support::recipe_fixture {

void test_check_program_authority()
{
  const profile_catalog catalog = profiles();
  source_snapshot snapshot = seal_source(
      source_origin("recipe.yml"), declaration_with_check(), catalog);
  assert(snapshot.recipe().check_program());
  assert(snapshot.recipe().check_program()->material() ==
         "meson test -C build\n");
  assert(snapshot.recipe().check_requirements().size() == 1);
  assert(snapshot.identity().hex() ==
         "12052c8b6bd1cb8ece3cdbf0c625cdcc79db35007386e818b884362eb720739c");

  source_snapshot changed =
      seal_source(source_origin("recipe.yml"),
                  declaration_with_check("ctest --test-dir build\n"),
                  catalog);
  assert(snapshot.identity() != changed.identity());

  source_snapshot without_requirements =
      seal_source(source_origin("recipe.yml"),
                  declaration_with_check("meson test -C build\n", false),
                  catalog);
  assert(without_requirements.recipe().check_program());
  assert(without_requirements.recipe().check_requirements().empty());

  source_snapshot first_origin =
      seal_source(source_origin("first.yml"), declaration(), catalog);
  source_snapshot second_origin =
      seal_source(source_origin("second.yml"), declaration(), catalog);
  assert(first_origin.identity() == second_origin.identity());
}

} // namespace test_support::recipe_fixture

int main()
{
  test_support::recipe_fixture::test_check_program_authority();
}
