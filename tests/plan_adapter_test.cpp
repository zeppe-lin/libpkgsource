// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource-plan/adapter.h>

#include <cassert>
#include <string>

using namespace pkgsource;

namespace {

declaration_provenance at(const char* path, std::uint32_t line)
{
  return declaration_provenance("recipe.yml", path, line, 3);
}

source_snapshot snapshot(const char* check_script = "echo check\n")
{
  const profile_catalog catalog = profile_catalog::seal({
      profile_declaration(
          profile_reference("@toolchain"),
          declaration_provenance("profiles.yml", "toolchain", 1, 1),
          {profile_member_declaration(
              requirement_subject(package_reference("gcc")),
              declaration_provenance("profiles.yml", "toolchain[0]", 2, 3))}),
  });
  return seal_source(
      source_origin("recipe.yml"), source_syntax::recipe_yaml_v2,
      recipe_declaration(
          package_release(package_reference("example"), "1.0", 1),
          package_metadata("Example", std::nullopt, std::nullopt, {"MIT"}),
          {}, program(program_language::posix_shell, "echo build\n"),
          {
            requirement_declaration(
                requirement_scope::build(),
                requirement_subject(profile_reference("@toolchain")),
                at("requirements.build[0]", 10)),
            requirement_declaration(
                requirement_scope::run(),
                requirement_subject(package_reference("libfoo")),
                at("requirements.run[0]", 12)),
            requirement_declaration(
                requirement_scope::check(),
                requirement_subject(package_reference("pkgcheck")),
                at("requirements.check[0]", 14)),
            requirement_declaration(
                requirement_scope::lifecycle(lifecycle_action::pre_remove),
                requirement_subject(package_reference("remove-helper")),
                at("requirements.lifecycle.pre-remove[0]", 16)),
          },
          {
            lifecycle_program(
                lifecycle_action::pre_install,
                program(program_language::posix_shell, "echo install\n")),
            lifecycle_program(
                lifecycle_action::pre_remove,
                program(program_language::posix_shell, "echo remove\n")),
          },
          architecture_requirements(
              {architecture_reference("aarch64")},
              {architecture_reference("x86_64")}),
          at("$", 1),
          program(program_language::posix_shell, check_script)),
      catalog);
}

} // namespace

int main()
{
  plan_adapter::candidate_projection projected =
      plan_adapter::project_candidate(snapshot());
  const auto& candidate = projected.candidate();
  assert(projected.source_identity() == projected.source().identity());
  assert(candidate.release().name() == "example");
  assert(candidate.release().version() == "1.0");
  assert(candidate.release().release() == "1");
  const auto& control = candidate.control_projection();
  assert(control.runtime_dependencies().size() == 1);
  assert(control.runtime_dependencies()[0].expression() == "libfoo");
  assert(control.removal_lifecycle().size() == 1);
  assert(control.removal_lifecycle()[0].material() == "echo remove\n");
  assert(control.target_profile().size() == 1);
  assert(control.target_profile()[0].name()
         == "pkgsource.target-architectures");
  assert(control.target_profile()[0].value() == "x86_64");

  plan_adapter::candidate_projection changed =
      plan_adapter::project_candidate(snapshot("meson test -C build\n"));
  assert(projected.source_identity() != changed.source_identity());
  assert(candidate.identity() == changed.candidate().identity());
}
