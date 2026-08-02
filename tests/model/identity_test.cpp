// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/model_fixture.h"

namespace test_support::model_fixture {

void test_identity_domains()
{
  package_reference package("pkg-config");
  profile_reference profile("@toolchain");
  architecture_reference architecture("x86_64");
  assert(package.name() == "pkg-config");
  assert(profile.name() == "@toolchain");
  assert(architecture.name() == "x86_64");

  expect(error_code::invalid_identity, [] {
    package_reference value("Pkg");
  });
  expect(error_code::invalid_identity, [] {
    profile_reference value("toolchain");
  });
  expect(error_code::invalid_identity, [] {
    architecture_reference value("x86/64");
  });
}

} // namespace test_support::model_fixture

int main()
{
  test_support::model_fixture::test_identity_domains();
}
