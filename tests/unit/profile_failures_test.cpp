// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/profile_fixture.h"

namespace test_support::profile_fixture {

void test_rejections()
{
  expect(error_code::unknown_profile, [] {
    (void)profile_catalog::seal({
        profile_declaration(
            profile_reference("@outer"),
            at("outer", 1),
            {profile_member_declaration(
                requirement_subject(profile_reference("@missing")),
                at("outer[0]", 2))}),
    });
  });

  expect(error_code::profile_cycle, [] {
    (void)profile_catalog::seal({
        profile_declaration(
            profile_reference("@a"),
            at("a", 1),
            {profile_member_declaration(
                requirement_subject(profile_reference("@b")), at("a[0]", 2))}),
        profile_declaration(
            profile_reference("@b"),
            at("b", 3),
            {profile_member_declaration(
                requirement_subject(profile_reference("@a")), at("b[0]", 4))}),
    });
  });

  expect(error_code::duplicate_declaration, [] {
    (void)profile_catalog::seal({
        profile_declaration(
            profile_reference("@dup"),
            at("dup", 1),
            {
                profile_member_declaration(
                    requirement_subject(package_reference("gcc")),
                    at("dup[0]", 2)),
                profile_member_declaration(
                    requirement_subject(package_reference("gcc")),
                    at("dup[1]", 3)),
            }),
    });
  });
}

} // namespace test_support::profile_fixture

int main()
{
  test_support::profile_fixture::test_rejections();
}
