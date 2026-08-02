// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/profile_fixture.h"

namespace test_support::profile_fixture {

void test_requirement_resolution_and_provenance()
{
  profile_catalog catalog = profile_catalog::seal({
      leaf("@compiler", "gcc", 10),
      profile_declaration(
          profile_reference("@toolchain"),
          at("profiles.toolchain", 1),
          {
              profile_member_declaration(
                  requirement_subject(profile_reference("@compiler")),
                  at("profiles.toolchain[0]", 2)),
              profile_member_declaration(
                  requirement_subject(package_reference("binutils")),
                  at("profiles.toolchain[1]", 3)),
          }),
  });

  sealed_requirement_set requirements = sealed_requirement_set::seal(
      {
          requirement_declaration(
              requirement_scope::build(),
              requirement_subject(profile_reference("@toolchain")),
              declaration_provenance(
                  "recipe.yml", "requirements.build[0]", 12, 5)),
          requirement_declaration(
              requirement_scope::run(),
              requirement_subject(package_reference("libfoo")),
              declaration_provenance(
                  "recipe.yml", "requirements.run[0]", 15, 5)),
          requirement_declaration(
              requirement_scope::lifecycle(lifecycle_action::post_install),
              requirement_subject(package_reference("desktop-file-utils")),
              declaration_provenance("recipe.yml",
                                     "requirements.lifecycle.post-install[0]",
                                     18,
                                     7)),
      },
      catalog);

  const auto build = requirements.for_scope(requirement_scope::build());
  assert(build.size() == 2);
  assert(build[1].package().name() == "gcc");
  assert(build[1].origins()[0].expansion().size() == 2);
  assert(requirements.selected_build_profiles().size() == 1);
  assert(requirements.selected_build_profiles()[0].profile().name() ==
         "@toolchain");
  assert(requirements.profile_closure().size() == 2);
  assert(requirements.for_scope(requirement_scope::run()).size() == 1);
  assert(requirements
             .for_scope(
                 requirement_scope::lifecycle(lifecycle_action::post_install))
             .size() == 1);
}

} // namespace test_support::profile_fixture

int main()
{
  test_support::profile_fixture::test_requirement_resolution_and_provenance();
}
