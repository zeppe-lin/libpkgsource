// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/libpkgsource.h>

#include <cassert>
#include <functional>
#include <vector>

using namespace pkgsource;

namespace {

declaration_provenance at(const char* path, std::uint32_t line)
{
  return declaration_provenance("profiles.yml", path, line, 3);
}

template <typename Function>
void expect(error_code code, Function&& function)
{
  try {
    function();
    assert(false);
  } catch (const error& value) {
    assert(value.code() == code);
  }
}

profile_declaration leaf(const char* name, const char* package,
                         std::uint32_t line)
{
  return profile_declaration(
      profile_reference(name), at(name, line),
      {profile_member_declaration(
          requirement_subject(package_reference(package)),
          at(package, line + 1))});
}

void test_deterministic_nested_expansion()
{
  profile_declaration toolchain(
      profile_reference("@toolchain"), at("profiles.toolchain", 1),
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

void test_requirement_resolution_and_provenance()
{
  profile_catalog catalog = profile_catalog::seal({
      leaf("@compiler", "gcc", 10),
      profile_declaration(
          profile_reference("@toolchain"), at("profiles.toolchain", 1),
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
            declaration_provenance("recipe.yml", "requirements.build[0]", 12, 5)),
        requirement_declaration(
            requirement_scope::run(),
            requirement_subject(package_reference("libfoo")),
            declaration_provenance("recipe.yml", "requirements.run[0]", 15, 5)),
        requirement_declaration(
            requirement_scope::lifecycle(lifecycle_action::post_install),
            requirement_subject(package_reference("desktop-file-utils")),
            declaration_provenance("recipe.yml",
                                   "requirements.lifecycle.post-install[0]", 18, 7)),
      },
      catalog);

  const auto build = requirements.for_scope(requirement_scope::build());
  assert(build.size() == 2);
  assert(build[1].package().name() == "gcc");
  assert(build[1].origins()[0].expansion().size() == 2);
  assert(requirements.selected_build_profiles().size() == 1);
  assert(requirements.selected_build_profiles()[0].profile().name()
         == "@toolchain");
  assert(requirements.profile_closure().size() == 2);
  assert(requirements.for_scope(requirement_scope::run()).size() == 1);
  assert(requirements.for_scope(requirement_scope::lifecycle(
             lifecycle_action::post_install)).size() == 1);
}


void test_nested_identity_change()
{
  const profile_declaration outer(
      profile_reference("@toolchain"), at("toolchain", 1),
      {profile_member_declaration(
          requirement_subject(profile_reference("@compiler")),
          at("toolchain[0]", 2))});
  profile_catalog gcc = profile_catalog::seal({
      outer, leaf("@compiler", "gcc", 10),
  });
  profile_catalog clang = profile_catalog::seal({
      outer, leaf("@compiler", "clang", 10),
  });
  assert(gcc.require(profile_reference("@toolchain")).identity()
         != clang.require(profile_reference("@toolchain")).identity());
}

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
              declaration_provenance("recipe.yml", "requirements.build[0]", 1, 1)),
          requirement_declaration(
              requirement_scope::build(),
              requirement_subject(profile_reference("@compiler")),
              declaration_provenance("recipe.yml", "requirements.build[1]", 2, 1)),
        },
        catalog);
  });
}

void test_rejections()
{
  expect(error_code::unknown_profile, [] {
    (void)profile_catalog::seal({
        profile_declaration(
            profile_reference("@outer"), at("outer", 1),
            {profile_member_declaration(
                requirement_subject(profile_reference("@missing")),
                at("outer[0]", 2))}),
    });
  });

  expect(error_code::profile_cycle, [] {
    (void)profile_catalog::seal({
        profile_declaration(
            profile_reference("@a"), at("a", 1),
            {profile_member_declaration(
                requirement_subject(profile_reference("@b")), at("a[0]", 2))}),
        profile_declaration(
            profile_reference("@b"), at("b", 3),
            {profile_member_declaration(
                requirement_subject(profile_reference("@a")), at("b[0]", 4))}),
    });
  });

  expect(error_code::duplicate_declaration, [] {
    (void)profile_catalog::seal({
        profile_declaration(
            profile_reference("@dup"), at("dup", 1),
            {
              profile_member_declaration(
                  requirement_subject(package_reference("gcc")), at("dup[0]", 2)),
              profile_member_declaration(
                  requirement_subject(package_reference("gcc")), at("dup[1]", 3)),
            }),
    });
  });
}

} // namespace

int main()
{
  test_deterministic_nested_expansion();
  test_requirement_resolution_and_provenance();
  test_nested_identity_change();
  test_duplicate_requirement_rejection();
  test_rejections();
}
