// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/recipe_fixture.h"

namespace test_support::recipe_fixture {

void test_rejections()
{
  const profile_catalog catalog = profiles();
  expect(error_code::duplicate_declaration, [&] {
    recipe_declaration value = declaration();
    const std::string hash(64, 'c');
    std::vector<source_input> sources = value.sources();
    sources.push_back(
        source_input::remote("https://other.invalid/source",
                             "example.conf",
                             digest(digest_algorithm::sha256, hash)));
    (void)seal_recipe(recipe_declaration(value.release(),
                                         value.metadata(),
                                         std::move(sources),
                                         value.build_program(),
                                         value.requirements(),
                                         value.lifecycle_programs(),
                                         value.architectures(),
                                         value.provenance()),
                      catalog);
  });

  expect(error_code::invalid_recipe, [&] {
    recipe_declaration value = declaration();
    (void)seal_recipe(recipe_declaration(value.release(),
                                         value.metadata(),
                                         value.sources(),
                                         value.build_program(),
                                         value.requirements(),
                                         {},
                                         value.architectures(),
                                         value.provenance()),
                      catalog);
  });

  expect(error_code::invalid_recipe, [&] {
    recipe_declaration value = declaration();
    std::vector<requirement_declaration> requirements = value.requirements();
    requirements.emplace_back(
        requirement_scope::check(),
        requirement_subject(package_reference("pkgcheck")),
        at("recipe.yml", "requirements.check[0]", 18));
    (void)seal_source(source_origin("recipe.yml"),
                      recipe_declaration(value.release(),
                                         value.metadata(),
                                         value.sources(),
                                         value.build_program(),
                                         std::move(requirements),
                                         value.lifecycle_programs(),
                                         value.architectures(),
                                         value.provenance()),
                      catalog);
  });
}

} // namespace test_support::recipe_fixture

int main()
{
  test_support::recipe_fixture::test_rejections();
}
