// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file recipe.h
 *  \brief Native recipe declarations and sealed semantic authority.
 */
#pragma once

#include <libpkgsource/export.h>
#include <libpkgsource/profile.h>

#include <optional>
#include <vector>

namespace pkgsource {

/*! \brief Parser-neutral declaration of one complete native recipe. */
class PKGSOURCE_API recipe_declaration final {
public:
  recipe_declaration(package_release release,
                     package_metadata metadata,
                     std::vector<source_input> sources,
                     program build_program,
                     std::vector<requirement_declaration> requirements,
                     std::vector<lifecycle_program> lifecycle_programs,
                     architecture_requirements architectures,
                     declaration_provenance provenance);
  recipe_declaration(package_release release,
                     package_metadata metadata,
                     std::vector<source_input> sources,
                     program build_program,
                     std::vector<requirement_declaration> requirements,
                     std::vector<lifecycle_program> lifecycle_programs,
                     architecture_requirements architectures,
                     declaration_provenance provenance,
                     std::optional<program> check_program);
  [[nodiscard]] const package_release& release() const noexcept;
  [[nodiscard]] const package_metadata& metadata() const noexcept;
  [[nodiscard]] const std::vector<source_input>& sources() const noexcept;
  [[nodiscard]] const program& build_program() const noexcept;
  [[nodiscard]] const std::optional<program>& check_program() const noexcept;
  [[nodiscard]] const std::vector<requirement_declaration>&
  requirements() const noexcept;
  [[nodiscard]] const std::vector<lifecycle_program>&
  lifecycle_programs() const noexcept;
  [[nodiscard]] const architecture_requirements& architectures() const noexcept;
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

private:
  package_release release_;
  package_metadata metadata_;
  std::vector<source_input> sources_;
  program build_program_;
  std::optional<program> check_program_;
  std::vector<requirement_declaration> requirements_;
  std::vector<lifecycle_program> lifecycle_programs_;
  architecture_requirements architectures_;
  declaration_provenance provenance_;
};

/*! \brief Complete normalized recipe authority. */
class PKGSOURCE_API sealed_recipe final {
public:
  sealed_recipe(package_release release,
                package_metadata metadata,
                std::vector<source_input> sources,
                program build_program,
                sealed_requirement_set requirements,
                std::vector<lifecycle_program> lifecycle_programs,
                architecture_requirements architectures,
                declaration_provenance provenance);
  sealed_recipe(package_release release,
                package_metadata metadata,
                std::vector<source_input> sources,
                program build_program,
                sealed_requirement_set requirements,
                std::vector<lifecycle_program> lifecycle_programs,
                architecture_requirements architectures,
                declaration_provenance provenance,
                std::optional<program> check_program);
  [[nodiscard]] const package_release& release() const noexcept;
  [[nodiscard]] const package_metadata& metadata() const noexcept;
  [[nodiscard]] const std::vector<source_input>& sources() const noexcept;
  [[nodiscard]] const program& build_program() const noexcept;
  [[nodiscard]] const std::optional<program>& check_program() const noexcept;
  [[nodiscard]] const sealed_requirement_set& requirements() const noexcept;
  [[nodiscard]] std::vector<resolved_requirement> build_requirements() const;
  [[nodiscard]] std::vector<resolved_requirement> run_requirements() const;
  [[nodiscard]] std::vector<resolved_requirement> check_requirements() const;
  [[nodiscard]] std::vector<resolved_requirement>
  lifecycle_requirements(lifecycle_action action) const;
  [[nodiscard]] const std::vector<selected_profile>&
  selected_build_profiles() const noexcept;
  [[nodiscard]] const std::vector<sealed_profile>&
  profile_closure() const noexcept;
  [[nodiscard]] const std::vector<lifecycle_program>&
  lifecycle_programs() const noexcept;
  [[nodiscard]] const lifecycle_program*
  lifecycle(lifecycle_action action) const noexcept;
  [[nodiscard]] const architecture_requirements& architectures() const noexcept;
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

private:
  package_release release_;
  package_metadata metadata_;
  std::vector<source_input> sources_;
  program build_program_;
  std::optional<program> check_program_;
  sealed_requirement_set requirements_;
  std::vector<lifecycle_program> lifecycle_programs_;
  architecture_requirements architectures_;
  declaration_provenance provenance_;
};

/*! \brief Validate, expand, normalize, and seal one native recipe. */
[[nodiscard]] PKGSOURCE_API sealed_recipe
seal_recipe(recipe_declaration declaration, const profile_catalog& profiles);

} // namespace pkgsource
