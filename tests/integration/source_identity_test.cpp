// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/recipe_fixture.h"

namespace test_support::recipe_fixture {

void test_identity_field_sensitivity()
{
  const profile_catalog catalog = profiles();
  const recipe_declaration base = declaration();
  const auto baseline =
      seal_source(source_origin("recipe.yml"), base, catalog).identity();

  const auto changed_release =
      seal_source(source_origin("recipe.yml"),
                  recipe_declaration(
                      package_release(package_reference("example"), "1.2.4", 1),
                      base.metadata(),
                      base.sources(),
                      base.build_program(),
                      base.requirements(),
                      base.lifecycle_programs(),
                      base.architectures(),
                      base.provenance()),
                  catalog);
  assert(changed_release.identity() != baseline);

  const auto changed_metadata =
      seal_source(source_origin("recipe.yml"),
                  recipe_declaration(base.release(),
                                     package_metadata("Different summary",
                                                      "Long description",
                                                      "https://example.invalid",
                                                      {"MIT"}),
                                     base.sources(),
                                     base.build_program(),
                                     base.requirements(),
                                     base.lifecycle_programs(),
                                     base.architectures(),
                                     base.provenance()),
                  catalog);
  assert(changed_metadata.identity() != baseline);

  auto changed_sources = base.sources();
  changed_sources[0] = source_input::remote(
      "https://example.invalid/example.tar.xz",
      "example.tar.xz",
      digest(digest_algorithm::sha256, std::string(64, 'c')));
  const auto changed_source =
      seal_source(source_origin("recipe.yml"),
                  recipe_declaration(base.release(),
                                     base.metadata(),
                                     std::move(changed_sources),
                                     base.build_program(),
                                     base.requirements(),
                                     base.lifecycle_programs(),
                                     base.architectures(),
                                     base.provenance()),
                  catalog);
  assert(changed_source.identity() != baseline);

  auto changed_unpack_sources = base.sources();
  changed_unpack_sources[0] = source_input::remote(
      "https://example.invalid/example.tar.xz", "example.tar.xz",
      digest(digest_algorithm::sha256, std::string(64, 'a')),
      source_unpack_kind::archive);
  const auto changed_unpack =
      seal_source(source_origin("recipe.yml"),
                  recipe_declaration(base.release(), base.metadata(),
                                     std::move(changed_unpack_sources),
                                     base.build_program(), base.requirements(),
                                     base.lifecycle_programs(), base.architectures(),
                                     base.provenance()),
                  catalog);
  assert(changed_unpack.identity() != baseline);

  auto changed_requirements = base.requirements();
  changed_requirements.emplace_back(
      requirement_scope::run(),
      requirement_subject(package_reference("libbar")),
      at("recipe.yml", "requirements.run[1]", 16));
  const auto changed_requirement =
      seal_source(source_origin("recipe.yml"),
                  recipe_declaration(base.release(),
                                     base.metadata(),
                                     base.sources(),
                                     base.build_program(),
                                     std::move(changed_requirements),
                                     base.lifecycle_programs(),
                                     base.architectures(),
                                     base.provenance()),
                  catalog);
  assert(changed_requirement.identity() != baseline);

  const auto changed_lifecycle = seal_source(
      source_origin("recipe.yml"),
      recipe_declaration(
          base.release(),
          base.metadata(),
          base.sources(),
          base.build_program(),
          base.requirements(),
          {lifecycle_program(lifecycle_action::post_install,
                             program(program_language::posix_shell,
                                     "update-desktop-database --verbose\n"))},
          base.architectures(),
          base.provenance()),
      catalog);
  assert(changed_lifecycle.identity() != baseline);

  const auto changed_architecture =
      seal_source(source_origin("recipe.yml"),
                  recipe_declaration(base.release(),
                                     base.metadata(),
                                     base.sources(),
                                     base.build_program(),
                                     base.requirements(),
                                     base.lifecycle_programs(),
                                     architecture_requirements(
                                         {architecture_reference("x86_64")},
                                         {architecture_reference("aarch64")}),
                                     base.provenance()),
                  catalog);
  assert(changed_architecture.identity() != baseline);
}

} // namespace test_support::recipe_fixture

int main()
{
  test_support::recipe_fixture::test_identity_field_sensitivity();
}
