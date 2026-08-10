// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/profile_fixture.h"

namespace test_support::profile_fixture {

void test_nested_identity_change()
{
  const profile_declaration outer(
      profile_reference("@toolchain"),
      at("toolchain", 1),
      {profile_member_declaration(
          requirement_subject(profile_reference("@compiler")),
          at("toolchain[0]", 2))});
  profile_catalog gcc = profile_catalog::seal({
      outer,
      leaf("@compiler", "gcc", 10),
  });
  profile_catalog clang = profile_catalog::seal({
      outer,
      leaf("@compiler", "clang", 10),
  });
  assert(gcc.require(profile_reference("@toolchain")).identity() !=
         clang.require(profile_reference("@toolchain")).identity());
}

} // namespace test_support::profile_fixture

int main()
{
  test_support::profile_fixture::test_nested_identity_change();
}
