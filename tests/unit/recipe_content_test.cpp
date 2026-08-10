// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/recipe_fixture.h"

namespace test_support::recipe_fixture {

void test_complete_snapshot()
{
  const profile_catalog catalog = profiles();
  source_snapshot snapshot =
      seal_source(source_origin("recipe.yml"), declaration(), catalog);
  const sealed_recipe& recipe = snapshot.recipe();
  assert(recipe.release().package().name() == "example");
  assert(recipe.sources().size() == 2);
  assert(recipe.sources()[0].local_name() == "example.conf");
  assert(recipe.build_requirements().size() == 2);
  assert(recipe.run_requirements().size() == 1);
  assert(recipe.check_requirements().empty());
  assert(!recipe.check_program());
  assert(recipe.lifecycle_requirements(lifecycle_action::post_install).size() ==
         1);
  assert(recipe.selected_build_profiles().size() == 1);
  assert(recipe.profile_closure().size() == 2);
  assert(recipe.lifecycle(lifecycle_action::post_install) != nullptr);
  assert(recipe.lifecycle(lifecycle_action::pre_remove) == nullptr);
  assert(recipe.architectures().build()[0].name() == "x86_64");
  assert(snapshot.identity().hex() ==
         "9dcbc183f1b42feaa33152d24eb559d60c2ea80b1f652a79f31e2dab18b99154");
}

} // namespace test_support::recipe_fixture

int main()
{
  test_support::recipe_fixture::test_complete_snapshot();
}
