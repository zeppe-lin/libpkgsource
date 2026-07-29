// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/recipe.h>

#include <libpkgsource/error.h>

#include "identity_support.h"

#include <algorithm>
#include <set>
#include <tuple>
#include <utility>

namespace pkgsource {
namespace {

void write_optional(detail::identity_writer& writer,
                    const std::optional<std::string>& value)
{
  writer.number(value.has_value() ? 1 : 0);
  if (value)
    writer.text(*value);
}

void write_scope(detail::identity_writer& writer,
                 const requirement_scope& scope)
{
  writer.text(to_string(scope.kind()));
  if (scope.action())
    writer.text(to_string(*scope.action()));
}

recipe_identity make_recipe_identity(
    const package_release& release,
    const package_metadata& metadata,
    const std::vector<source_input>& sources,
    const program& build_program,
    const std::optional<program>& check_program,
    const sealed_requirement_set& requirements,
    const std::vector<lifecycle_program>& lifecycle_programs,
    const architecture_requirements& architectures)
{
  detail::identity_writer writer;
  writer.text(check_program ? "libpkgsource/recipe/v2"
                            : "libpkgsource/recipe/v1");
  writer.text(release.identity().hex());
  writer.text(metadata.summary());
  write_optional(writer, metadata.description());
  write_optional(writer, metadata.homepage());
  writer.number(metadata.licenses().size());
  for (const std::string& license : metadata.licenses())
    writer.text(license);

  writer.number(sources.size());
  for (const source_input& source : sources) {
    writer.text(to_string(source.kind()));
    writer.text(source.location());
    writer.text(source.local_name());
    writer.text(to_string(source.content_digest().algorithm()));
    writer.text(source.content_digest().hex());
  }

  writer.text(to_string(build_program.language()));
  writer.text(build_program.material());
  if (check_program) {
    writer.text(to_string(check_program->language()));
    writer.text(check_program->material());
  }

  writer.number(requirements.requirements().size());
  for (const resolved_requirement& requirement : requirements.requirements()) {
    write_scope(writer, requirement.scope());
    writer.text(requirement.package().name());
    writer.number(requirement.origins().size());
    for (const requirement_origin& origin : requirement.origins()) {
      writer.number(origin.expansion().size());
      for (const profile_expansion_step& step : origin.expansion()) {
        writer.text(step.profile().name());
        writer.text(to_string(step.member().kind()));
        writer.text(step.member().text());
      }
    }
  }

  writer.number(requirements.profile_closure().size());
  for (const sealed_profile& profile : requirements.profile_closure()) {
    writer.text(profile.name().name());
    writer.text(profile.identity().hex());
  }

  writer.number(lifecycle_programs.size());
  for (const lifecycle_program& lifecycle : lifecycle_programs) {
    writer.text(to_string(lifecycle.action()));
    writer.text(to_string(lifecycle.value().language()));
    writer.text(lifecycle.value().material());
  }

  writer.number(architectures.build().size());
  for (const architecture_reference& architecture : architectures.build())
    writer.text(architecture.name());
  writer.number(architectures.target().size());
  for (const architecture_reference& architecture : architectures.target())
    writer.text(architecture.name());

  return recipe_identity::from_sha256(writer.finish());
}

} // namespace

recipe_declaration::recipe_declaration(
    package_release release, package_metadata metadata,
    std::vector<source_input> sources, program build_program,
    std::vector<requirement_declaration> requirements,
    std::vector<lifecycle_program> lifecycle_programs,
    architecture_requirements architectures,
    declaration_provenance provenance)
    : recipe_declaration(
          std::move(release), std::move(metadata), std::move(sources),
          std::move(build_program), std::move(requirements),
          std::move(lifecycle_programs), std::move(architectures),
          std::move(provenance), std::nullopt)
{
}

recipe_declaration::recipe_declaration(
    package_release release, package_metadata metadata,
    std::vector<source_input> sources, program build_program,
    std::vector<requirement_declaration> requirements,
    std::vector<lifecycle_program> lifecycle_programs,
    architecture_requirements architectures,
    declaration_provenance provenance,
    std::optional<program> check_program)
    : release_(std::move(release)), metadata_(std::move(metadata)),
      sources_(std::move(sources)), build_program_(std::move(build_program)),
      check_program_(std::move(check_program)),
      requirements_(std::move(requirements)),
      lifecycle_programs_(std::move(lifecycle_programs)),
      architectures_(std::move(architectures)),
      provenance_(std::move(provenance))
{
}
const package_release& recipe_declaration::release() const noexcept { return release_; }
const package_metadata& recipe_declaration::metadata() const noexcept { return metadata_; }
const std::vector<source_input>& recipe_declaration::sources() const noexcept
{
  return sources_;
}
const program& recipe_declaration::build_program() const noexcept
{
  return build_program_;
}
const std::optional<program>& recipe_declaration::check_program() const noexcept
{
  return check_program_;
}
const std::vector<requirement_declaration>&
recipe_declaration::requirements() const noexcept { return requirements_; }
const std::vector<lifecycle_program>&
recipe_declaration::lifecycle_programs() const noexcept
{
  return lifecycle_programs_;
}
const architecture_requirements&
recipe_declaration::architectures() const noexcept { return architectures_; }
const declaration_provenance& recipe_declaration::provenance() const noexcept
{
  return provenance_;
}

sealed_recipe::sealed_recipe(
    package_release release, package_metadata metadata,
    std::vector<source_input> sources, program build_program,
    sealed_requirement_set requirements,
    std::vector<lifecycle_program> lifecycle_programs,
    architecture_requirements architectures,
    declaration_provenance provenance, recipe_identity identity)
    : sealed_recipe(
          std::move(release), std::move(metadata), std::move(sources),
          std::move(build_program), std::move(requirements),
          std::move(lifecycle_programs), std::move(architectures),
          std::move(provenance), std::move(identity), std::nullopt)
{
}

sealed_recipe::sealed_recipe(
    package_release release, package_metadata metadata,
    std::vector<source_input> sources, program build_program,
    sealed_requirement_set requirements,
    std::vector<lifecycle_program> lifecycle_programs,
    architecture_requirements architectures,
    declaration_provenance provenance, recipe_identity identity,
    std::optional<program> check_program)
    : release_(std::move(release)), metadata_(std::move(metadata)),
      sources_(std::move(sources)), build_program_(std::move(build_program)),
      check_program_(std::move(check_program)),
      requirements_(std::move(requirements)),
      lifecycle_programs_(std::move(lifecycle_programs)),
      architectures_(std::move(architectures)),
      provenance_(std::move(provenance)), identity_(std::move(identity))
{
}
const package_release& sealed_recipe::release() const noexcept { return release_; }
const package_metadata& sealed_recipe::metadata() const noexcept { return metadata_; }
const std::vector<source_input>& sealed_recipe::sources() const noexcept
{
  return sources_;
}
const program& sealed_recipe::build_program() const noexcept { return build_program_; }
const std::optional<program>& sealed_recipe::check_program() const noexcept
{
  return check_program_;
}
const sealed_requirement_set& sealed_recipe::requirements() const noexcept
{
  return requirements_;
}
std::vector<resolved_requirement> sealed_recipe::build_requirements() const
{
  return requirements_.for_scope(requirement_scope::build());
}
std::vector<resolved_requirement> sealed_recipe::run_requirements() const
{
  return requirements_.for_scope(requirement_scope::run());
}
std::vector<resolved_requirement> sealed_recipe::check_requirements() const
{
  return requirements_.for_scope(requirement_scope::check());
}
std::vector<resolved_requirement> sealed_recipe::lifecycle_requirements(
    lifecycle_action action) const
{
  return requirements_.for_scope(requirement_scope::lifecycle(action));
}
const std::vector<selected_profile>&
sealed_recipe::selected_build_profiles() const noexcept
{
  return requirements_.selected_build_profiles();
}
const std::vector<sealed_profile>& sealed_recipe::profile_closure() const noexcept
{
  return requirements_.profile_closure();
}
const std::vector<lifecycle_program>&
sealed_recipe::lifecycle_programs() const noexcept
{
  return lifecycle_programs_;
}
const lifecycle_program* sealed_recipe::lifecycle(
    lifecycle_action action) const noexcept
{
  const auto found = std::lower_bound(
      lifecycle_programs_.begin(), lifecycle_programs_.end(), action,
      [](const lifecycle_program& value, lifecycle_action key) {
        return value.action() < key;
      });
  return found != lifecycle_programs_.end() && found->action() == action
      ? &*found : nullptr;
}
const architecture_requirements& sealed_recipe::architectures() const noexcept
{
  return architectures_;
}
const declaration_provenance& sealed_recipe::provenance() const noexcept
{
  return provenance_;
}
const recipe_identity& sealed_recipe::identity() const noexcept { return identity_; }

sealed_recipe seal_recipe(recipe_declaration declaration,
                          const profile_catalog& profiles)
{
  std::vector<source_input> sources = declaration.sources();
  std::sort(sources.begin(), sources.end());
  for (std::size_t i = 1; i < sources.size(); ++i)
    if (sources[i - 1].local_name() == sources[i].local_name())
      throw error(error_code::duplicate_declaration,
                  "duplicate source local name: " + sources[i].local_name());

  std::vector<lifecycle_program> lifecycle = declaration.lifecycle_programs();
  std::sort(lifecycle.begin(), lifecycle.end());
  for (std::size_t i = 1; i < lifecycle.size(); ++i)
    if (lifecycle[i - 1].action() == lifecycle[i].action())
      throw error(error_code::duplicate_declaration,
                  "duplicate lifecycle program: "
                      + std::string(to_string(lifecycle[i].action())));

  sealed_requirement_set requirements = sealed_requirement_set::seal(
      declaration.requirements(), profiles);
  for (const resolved_requirement& requirement : requirements.requirements()) {
    if (requirement.scope().kind() != requirement_scope_kind::lifecycle)
      continue;
    const lifecycle_action action = *requirement.scope().action();
    const bool present = std::any_of(
        lifecycle.begin(), lifecycle.end(),
        [action](const lifecycle_program& value) {
          return value.action() == action;
        });
    if (!present)
      throw error(error_code::invalid_recipe,
                  "lifecycle requirements without program: "
                      + std::string(to_string(action)));
  }

  const recipe_identity identity = make_recipe_identity(
      declaration.release(), declaration.metadata(), sources,
      declaration.build_program(), declaration.check_program(), requirements,
      lifecycle, declaration.architectures());

  return sealed_recipe(
      declaration.release(), declaration.metadata(), std::move(sources),
      declaration.build_program(), std::move(requirements),
      std::move(lifecycle), declaration.architectures(),
      declaration.provenance(), identity, declaration.check_program());
}

} // namespace pkgsource
