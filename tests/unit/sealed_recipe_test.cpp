// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/recipe_fixture.h"

namespace test_support::recipe_fixture {

void test_reconstruction_authentication()
{
  const profile_catalog catalog = profiles();
  const sealed_recipe valid = seal_recipe(declaration(), catalog);

  auto reversed_sources = valid.sources();
  std::reverse(reversed_sources.begin(), reversed_sources.end());
  expect(error_code::invalid_recipe, [&] {
    (void)sealed_recipe(valid.release(),
                        valid.metadata(),
                        reversed_sources,
                        valid.build_program(),
                        valid.requirements(),
                        valid.lifecycle_programs(),
                        valid.architectures(),
                        valid.provenance(),
                        valid.check_program());
  });

  const recipe_declaration checked = declaration_with_check();
  const sealed_requirement_set requirements = sealed_requirement_set::seal(
      checked.requirements(), catalog);
  auto sources = checked.sources();
  std::sort(sources.begin(), sources.end());
  auto lifecycle = checked.lifecycle_programs();
  std::sort(lifecycle.begin(), lifecycle.end());
  expect(error_code::invalid_recipe, [&] {
    (void)sealed_recipe(checked.release(),
                        checked.metadata(),
                        sources,
                        checked.build_program(),
                        requirements,
                        lifecycle,
                        checked.architectures(),
                        checked.provenance());
  });
}

} // namespace test_support::recipe_fixture

int main()
{
  test_support::recipe_fixture::test_reconstruction_authentication();
}
