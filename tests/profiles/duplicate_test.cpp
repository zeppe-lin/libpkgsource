// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/profile_fixture.h"

namespace test_support::profile_fixture {

void test_duplicate_requirement_rejection()
{
  const profile_catalog catalog = profile_catalog::seal({
      leaf("@compiler", "gcc", 10),
  });
  expect(error_code::duplicate_declaration, [&] {
    (void)sealed_requirement_set::seal(
        {
            requirement_declaration(
                requirement_scope::build(),
                requirement_subject(profile_reference("@compiler")),
                declaration_provenance(
                    "recipe.yml", "requirements.build[0]", 1, 1)),
            requirement_declaration(
                requirement_scope::build(),
                requirement_subject(profile_reference("@compiler")),
                declaration_provenance(
                    "recipe.yml", "requirements.build[1]", 2, 1)),
        },
        catalog);
  });
}

} // namespace test_support::profile_fixture

int main()
{
  test_support::profile_fixture::test_duplicate_requirement_rejection();
}
