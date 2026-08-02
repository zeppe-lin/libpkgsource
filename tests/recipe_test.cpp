// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/libpkgsource.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <optional>
#include <string>
#include <vector>

using namespace pkgsource;

namespace {

declaration_provenance at(const char* document, const char* path,
                          std::uint32_t line)
{
  return declaration_provenance(document, path, line, 3);
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

profile_catalog profiles()
{
  return profile_catalog::seal({
      profile_declaration(
          profile_reference("@compiler"), at("profiles.yml", "compiler", 1),
          {profile_member_declaration(
              requirement_subject(package_reference("gcc")),
              at("profiles.yml", "compiler[0]", 2))}),
      profile_declaration(
          profile_reference("@toolchain"), at("profiles.yml", "toolchain", 4),
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
      source_input::local("files/example.conf", "example.conf",
                          digest(digest_algorithm::sha256,
                                 std::string(64, 'b'))),
  };
  std::vector<requirement_declaration> requirements = {
      requirement_declaration(
          requirement_scope::build(),
          requirement_subject(profile_reference("@toolchain")),
          at("recipe.yml", "requirements.build[0]", 12)),
      requirement_declaration(
          requirement_scope::run(),
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
      package_metadata("Example package", "Long description",
                       "https://example.invalid", {"MIT"}),
      std::move(sources),
      program(program_language::posix_shell, build_script),
      std::move(requirements),
      {lifecycle_program(
          lifecycle_action::post_install,
          program(program_language::posix_shell,
                  "update-desktop-database\n"))},
      architecture_requirements(
          {architecture_reference("x86_64")},
          {architecture_reference("x86_64")}),
      at("recipe.yml", "$", 1));
}


recipe_declaration declaration_with_check(
    const char* check_script = "meson test -C build\n",
    bool include_check_requirement = true)
{
  recipe_declaration value = declaration();
  std::vector<requirement_declaration> requirements = value.requirements();
  if (include_check_requirement)
    requirements.emplace_back(
        requirement_scope::check(),
        requirement_subject(package_reference("pkgcheck")),
        at("recipe.yml", "requirements.check[0]", 18));
  return recipe_declaration(
      value.release(), value.metadata(), value.sources(), value.build_program(),
      std::move(requirements), value.lifecycle_programs(),
      value.architectures(), value.provenance(),
      program(program_language::posix_shell, check_script));
}

void test_complete_snapshot()
{
  const profile_catalog catalog = profiles();
  source_snapshot snapshot = seal_source(
      source_origin("recipe.yml"),
      declaration(), catalog);
  const sealed_recipe& recipe = snapshot.recipe();
  assert(recipe.release().package().name() == "example");
  assert(recipe.sources().size() == 2);
  assert(recipe.sources()[0].local_name() == "example.conf");
  assert(recipe.build_requirements().size() == 2);
  assert(recipe.run_requirements().size() == 1);
  assert(recipe.check_requirements().empty());
  assert(!recipe.check_program());
  assert(recipe.lifecycle_requirements(
             lifecycle_action::post_install).size() == 1);
  assert(recipe.selected_build_profiles().size() == 1);
  assert(recipe.profile_closure().size() == 2);
  assert(recipe.lifecycle(lifecycle_action::post_install) != nullptr);
  assert(recipe.lifecycle(lifecycle_action::pre_remove) == nullptr);
  assert(recipe.architectures().build()[0].name() == "x86_64");
  assert(snapshot.identity().hex()
         == "9dcbc183f1b42feaa33152d24eb559d60c2ea80b1f652a79f31e2dab18b99154");
}

void test_identity_normalization()
{
  const profile_catalog catalog = profiles();
  source_snapshot first = seal_source(
      source_origin("a.yml"),
      declaration(false), catalog);
  source_snapshot reordered = seal_source(
      source_origin("b.yml"),
      declaration(true), catalog);
  source_snapshot changed = seal_source(
      source_origin("a.yml"),
      declaration(false, "echo changed\n"), catalog);
  assert(first.identity() == reordered.identity());
  assert(first.identity() != changed.identity());
}

void test_identity_field_sensitivity()
{
  const profile_catalog catalog = profiles();
  const recipe_declaration base = declaration();
  const auto baseline = seal_source(
      source_origin("recipe.yml"), base, catalog).identity();

  const auto changed_release = seal_source(
      source_origin("recipe.yml"),
      recipe_declaration(
          package_release(package_reference("example"), "1.2.4", 1),
          base.metadata(), base.sources(), base.build_program(),
          base.requirements(), base.lifecycle_programs(), base.architectures(),
          base.provenance()),
      catalog);
  assert(changed_release.identity() != baseline);

  const auto changed_metadata = seal_source(
      source_origin("recipe.yml"),
      recipe_declaration(
          base.release(),
          package_metadata("Different summary", "Long description",
                           "https://example.invalid", {"MIT"}),
          base.sources(), base.build_program(), base.requirements(),
          base.lifecycle_programs(), base.architectures(), base.provenance()),
      catalog);
  assert(changed_metadata.identity() != baseline);

  auto changed_sources = base.sources();
  changed_sources[0] = source_input::remote(
      "https://example.invalid/example.tar.xz", "example.tar.xz",
      digest(digest_algorithm::sha256, std::string(64, 'c')));
  const auto changed_source = seal_source(
      source_origin("recipe.yml"),
      recipe_declaration(
          base.release(), base.metadata(), std::move(changed_sources),
          base.build_program(), base.requirements(), base.lifecycle_programs(),
          base.architectures(), base.provenance()),
      catalog);
  assert(changed_source.identity() != baseline);

  auto changed_requirements = base.requirements();
  changed_requirements.emplace_back(
      requirement_scope::run(),
      requirement_subject(package_reference("libbar")),
      at("recipe.yml", "requirements.run[1]", 16));
  const auto changed_requirement = seal_source(
      source_origin("recipe.yml"),
      recipe_declaration(
          base.release(), base.metadata(), base.sources(), base.build_program(),
          std::move(changed_requirements), base.lifecycle_programs(),
          base.architectures(), base.provenance()),
      catalog);
  assert(changed_requirement.identity() != baseline);

  const auto changed_lifecycle = seal_source(
      source_origin("recipe.yml"),
      recipe_declaration(
          base.release(), base.metadata(), base.sources(), base.build_program(),
          base.requirements(),
          {lifecycle_program(
              lifecycle_action::post_install,
              program(program_language::posix_shell,
                      "update-desktop-database --verbose\n"))},
          base.architectures(), base.provenance()),
      catalog);
  assert(changed_lifecycle.identity() != baseline);

  const auto changed_architecture = seal_source(
      source_origin("recipe.yml"),
      recipe_declaration(
          base.release(), base.metadata(), base.sources(), base.build_program(),
          base.requirements(), base.lifecycle_programs(),
          architecture_requirements(
              {architecture_reference("x86_64")},
              {architecture_reference("aarch64")}),
          base.provenance()),
      catalog);
  assert(changed_architecture.identity() != baseline);
}

void test_check_program_authority()
{
  const profile_catalog catalog = profiles();
  source_snapshot snapshot = seal_source(
      source_origin("recipe.yml"),
      declaration_with_check(), catalog);
  assert(snapshot.recipe().check_program());
  assert(snapshot.recipe().check_program()->material()
         == "meson test -C build\n");
  assert(snapshot.recipe().check_requirements().size() == 1);
  assert(snapshot.identity().hex()
         == "b1f0b553c0c7dbd1692d6753eedc61708efa733417abec06bdf6d395ad3fa5ef");

  source_snapshot changed = seal_source(
      source_origin("recipe.yml"),
      declaration_with_check("ctest --test-dir build\n"), catalog);
  assert(snapshot.identity() != changed.identity());

  source_snapshot without_requirements = seal_source(
      source_origin("recipe.yml"),
      declaration_with_check("meson test -C build\n", false), catalog);
  assert(without_requirements.recipe().check_program());
  assert(without_requirements.recipe().check_requirements().empty());

  source_snapshot first_origin = seal_source(
      source_origin("first.yml"), declaration(), catalog);
  source_snapshot second_origin = seal_source(
      source_origin("second.yml"), declaration(), catalog);
  assert(first_origin.identity() == second_origin.identity());
}

void test_program_digest_vector()
{
  program value(program_language::posix_shell, "abc");
  assert(value.content_digest().hex()
         == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

void test_rejections()
{
  const profile_catalog catalog = profiles();
  expect(error_code::duplicate_declaration, [&] {
    recipe_declaration value = declaration();
    const std::string hash(64, 'c');
    std::vector<source_input> sources = value.sources();
    sources.push_back(source_input::remote(
        "https://other.invalid/source", "example.conf",
        digest(digest_algorithm::sha256, hash)));
    (void)seal_recipe(
        recipe_declaration(value.release(), value.metadata(), std::move(sources),
                           value.build_program(), value.requirements(),
                           value.lifecycle_programs(), value.architectures(),
                           value.provenance()),
        catalog);
  });

  expect(error_code::invalid_recipe, [&] {
    recipe_declaration value = declaration();
    (void)seal_recipe(
        recipe_declaration(value.release(), value.metadata(), value.sources(),
                           value.build_program(), value.requirements(), {},
                           value.architectures(), value.provenance()),
        catalog);
  });

  expect(error_code::invalid_recipe, [&] {
    recipe_declaration value = declaration();
    std::vector<requirement_declaration> requirements = value.requirements();
    requirements.emplace_back(
        requirement_scope::check(),
        requirement_subject(package_reference("pkgcheck")),
        at("recipe.yml", "requirements.check[0]", 18));
    (void)seal_source(
        source_origin("recipe.yml"),
        recipe_declaration(
            value.release(), value.metadata(), value.sources(),
            value.build_program(), std::move(requirements),
            value.lifecycle_programs(), value.architectures(),
            value.provenance()),
        catalog);
  });
}

} // namespace

int main()
{
  test_complete_snapshot();
  test_identity_normalization();
  test_identity_field_sensitivity();
  test_check_program_authority();
  test_program_digest_vector();
  test_rejections();
}
