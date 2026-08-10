// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource/libpkgsource.h>

#include <optional>

int main()
{
  using namespace pkgsource;
  const profile_catalog profiles = profile_catalog::seal({});
  const recipe_declaration declaration(
      package_release(package_reference("installed-consumer"), "1", 1),
      package_metadata("Installed consumer", std::nullopt, std::nullopt, {"MIT"}),
      {},
      program(program_language::posix_shell, "echo build\n"),
      {},
      {},
      architecture_requirements({}, {}),
      declaration_provenance("recipe.yml", "$", 1, 1));
  const source_snapshot snapshot =
      seal_source(source_origin("recipe.yml"), declaration, profiles);
  return snapshot.identity().hex().size() == 64 ? 0 : 1;
}
