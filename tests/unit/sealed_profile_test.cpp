// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/profile_fixture.h"

namespace test_support::profile_fixture {

void test_reconstruction_authentication()
{
  const profile_catalog catalog = profile_catalog::seal({
      leaf("@compiler", "gcc", 10),
  });
  const sealed_profile& valid = catalog.require(profile_reference("@compiler"));

  expect(error_code::invalid_identity, [&] {
    (void)sealed_profile(valid.name(),
                         profile_identity::from_sha256(std::string(64, '0')),
                         valid.provenance(),
                         valid.direct_members(),
                         valid.expansion());
  });

  auto forged_expansion = valid.expansion();
  assert(forged_expansion.size() == 1);
  const profile_expansion_step valid_step =
      forged_expansion.front().steps().front();
  forged_expansion.front() = profile_expansion_path(
      forged_expansion.front().package(),
      {profile_expansion_step(valid_step.profile(),
                              valid_step.member(),
                              declaration_provenance(
                                  "profiles.yml", "forged", 99, 1))});
  expect(error_code::invalid_profile, [&] {
    (void)sealed_profile(valid.name(),
                         valid.identity(),
                         valid.provenance(),
                         valid.direct_members(),
                         forged_expansion);
  });

  const declaration_provenance moved_member(
      "profiles-moved.yml", "compiler[0]", 40, 7);
  std::vector<profile_member_declaration> moved_members{
      profile_member_declaration(valid_step.member(), moved_member),
  };
  std::vector<profile_expansion_path> moved_expansion{
      profile_expansion_path(
          valid.expansion().front().package(),
          {profile_expansion_step(
              valid_step.profile(), valid_step.member(), moved_member)}),
  };
  const sealed_profile moved_provenance(
      valid.name(),
      valid.identity(),
      declaration_provenance("profiles-moved.yml", "compiler", 39, 1),
      std::move(moved_members),
      std::move(moved_expansion));
  assert(moved_provenance.identity() == valid.identity());
}

} // namespace test_support::profile_fixture

int main()
{
  test_support::profile_fixture::test_reconstruction_authentication();
}
