// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgsource/libpkgsource.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace test_support::recipe_fixture {

using namespace pkgsource;

declaration_provenance
at(const char* document, const char* path, std::uint32_t line)
{
  return declaration_provenance(document, path, line, 3);
}

template <typename Function> void expect(error_code code, Function&& function)
{
  try {
    function();
    assert(false);
  } catch (const error& value) {
    assert(value.code() == code);
  }
}

profile_catalog profiles()
{
  return profile_catalog::seal({
      profile_declaration(profile_reference("@compiler"),
                          at("profiles.yml", "compiler", 1),
                          {profile_member_declaration(
                              requirement_subject(package_reference("gcc")),
                              at("profiles.yml", "compiler[0]", 2))}),
      profile_declaration(
          profile_reference("@toolchain"),
          at("profiles.yml", "toolchain", 4),
          {
              profile_member_declaration(
                  requirement_subject(package_reference("binutils")),
                  at("profiles.yml", "toolchain[0]", 5)),
              profile_member_declaration(
                  requirement_subject(profile_reference("@compiler")),
                  at("profiles.yml", "toolchain[1]", 6)),
          }),
  });
}

recipe_declaration declaration(bool reverse = false,
                               const char* build_script = "echo build\n")
{
  const std::string hash(64, 'a');
  std::vector<source_input> sources = {
      source_input::remote("https://example.invalid/example.tar.xz",
                           "example.tar.xz",
                           digest(digest_algorithm::sha256, hash)),
      source_input::local(
          "files/example.conf",
          "example.conf",
          digest(digest_algorithm::sha256, std::string(64, 'b'))),
  };
  std::vector<requirement_declaration> requirements = {
      requirement_declaration(
          requirement_scope::build(),
          requirement_subject(profile_reference("@toolchain")),
          at("recipe.yml", "requirements.build[0]", 12)),
      requirement_declaration(requirement_scope::run(),
                              requirement_subject(package_reference("libfoo")),
                              at("recipe.yml", "requirements.run[0]", 15)),
      requirement_declaration(
          requirement_scope::lifecycle(lifecycle_action::post_install),
          requirement_subject(package_reference("desktop-file-utils")),
          at("recipe.yml", "requirements.lifecycle.post-install[0]", 21)),
  };
  if (reverse) {
    std::reverse(sources.begin(), sources.end());
    std::reverse(requirements.begin(), requirements.end());
  }
  return recipe_declaration(
      package_release(package_reference("example"), "1.2.3", 1),
      package_metadata("Example package",
                       "Long description",
                       "https://example.invalid",
                       {"MIT"}),
      std::move(sources),
      program(program_language::posix_shell, build_script),
      std::move(requirements),
      {lifecycle_program(
          lifecycle_action::post_install,
          program(program_language::posix_shell, "update-desktop-database\n"))},
      architecture_requirements({architecture_reference("x86_64")},
                                {architecture_reference("x86_64")}),
      at("recipe.yml", "$", 1));
}

recipe_declaration
declaration_with_check(const char* check_script = "meson test -C build\n",
                       bool include_check_requirement = true)
{
  recipe_declaration value = declaration();
  std::vector<requirement_declaration> requirements = value.requirements();
  if (include_check_requirement) {
    requirements.emplace_back(
        requirement_scope::check(),
        requirement_subject(package_reference("pkgcheck")),
        at("recipe.yml", "requirements.check[0]", 18));
  }
  return recipe_declaration(
      value.release(),
      value.metadata(),
      value.sources(),
      value.build_program(),
      std::move(requirements),
      value.lifecycle_programs(),
      value.architectures(),
      value.provenance(),
      program(program_language::posix_shell, check_script));
}

} // namespace test_support::recipe_fixture
