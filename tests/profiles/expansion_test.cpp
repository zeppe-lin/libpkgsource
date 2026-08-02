// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/profile_fixture.h"

namespace test_support::profile_fixture {

void test_deterministic_nested_expansion()
{
  profile_declaration toolchain(
      profile_reference("@toolchain"),
      at("profiles.toolchain", 1),
      {
          profile_member_declaration(
              requirement_subject(profile_reference("@compiler")),
              at("profiles.toolchain[0]", 2)),
          profile_member_declaration(
              requirement_subject(package_reference("binutils")),
              at("profiles.toolchain[1]", 3)),
      });
  profile_declaration compiler = leaf("@compiler", "gcc", 10);

  profile_catalog first = profile_catalog::seal({toolchain, compiler});
  profile_catalog second = profile_catalog::seal({compiler, toolchain});
  const sealed_profile& a = first.require(profile_reference("@toolchain"));
  const sealed_profile& b = second.require(profile_reference("@toolchain"));
  assert(a.identity() == b.identity());
  assert(a.expansion().size() == 2);
  assert(a.expansion()[0].package().name() == "binutils");
  assert(a.expansion()[1].package().name() == "gcc");
  assert(a.expansion()[1].steps().size() == 2);
  assert(a.expansion()[1].steps()[0].profile().name() == "@toolchain");
  assert(a.expansion()[1].steps()[1].profile().name() == "@compiler");
}

} // namespace test_support::profile_fixture

int main()
{
  test_support::profile_fixture::test_deterministic_nested_expansion();
}
