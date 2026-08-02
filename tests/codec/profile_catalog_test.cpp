// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/codec_fixture.h"

namespace test_support::codec_fixture {

void test_profile_catalog_round_trip()
{
  const auto catalog = profiles();
  const auto encoding = encode_profile_catalog(catalog);
  assert(encoding.size() > 42);
  assert(std::equal(
      encoding.begin(),
      encoding.begin() + 8,
      std::array<std::uint8_t, 8>{'Z', 'L', 'P', 'S', 'P', 'C', 'A', 'T'}
          .begin()));
  const auto decoded = decode_profile_catalog(encoding);
  assert(decoded.profiles().size() == 3);
  assert(decoded.require(profile_reference("@toolchain")).expansion().size() ==
         2);
  assert(decoded.require(profile_reference("@toolchain")).identity() ==
         catalog.require(profile_reference("@toolchain")).identity());
  assert(encode_profile_catalog(decoded) == encoding);

  auto reordered = profile_catalog::seal({
      profile_declaration(profile_reference("@runtime"),
                          at("profiles.yml", "runtime", 8),
                          {profile_member_declaration(
                              requirement_subject(package_reference("libfoo")),
                              at("profiles.yml", "runtime[0]", 9))}),
      profile_declaration(
          profile_reference("@toolchain"),
          at("profiles.yml", "toolchain", 4),
          {
              profile_member_declaration(
                  requirement_subject(profile_reference("@compiler")),
                  at("profiles.yml", "toolchain[1]", 6)),
              profile_member_declaration(
                  requirement_subject(package_reference("binutils")),
                  at("profiles.yml", "toolchain[0]", 5)),
          }),
      profile_declaration(profile_reference("@compiler"),
                          at("profiles.yml", "compiler", 1),
                          {profile_member_declaration(
                              requirement_subject(package_reference("gcc")),
                              at("profiles.yml", "compiler[0]", 2))}),
  });
  assert(encode_profile_catalog(reordered) == encoding);
}

} // namespace test_support::codec_fixture

int main()
{
  test_support::codec_fixture::test_profile_catalog_round_trip();
}
