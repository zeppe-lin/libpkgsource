// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/** \file recipe.h
 *  \brief Native recipe declarations and normalized semantic authority.
 */

#pragma once

#include <libpkgsource/export.h>
#include <libpkgsource/profile.h>

#include <optional>
#include <vector>

namespace pkgsource {

/** \brief Parser-neutral declaration of one complete native recipe. */
class PKGSOURCE_API recipe_declaration final {
public:
  /** Construct a recipe declaration without a check program.
   * \param release Package release coordinates.
   * \param metadata Package metadata.
   * \param sources Declared source inputs.
   * \param build_program Exact build program.
   * \param requirements Direct requirement declarations.
   * \param lifecycle_programs Action-bound lifecycle programs.
   * \param architectures Build and target architecture constraints.
   * \param provenance Exact recipe declaration site.
   */
  recipe_declaration(package_release release,
                     package_metadata metadata,
                     std::vector<source_input> sources,
                     program build_program,
                     std::vector<requirement_declaration> requirements,
                     std::vector<lifecycle_program> lifecycle_programs,
                     architecture_requirements architectures,
                     declaration_provenance provenance);

  /** Construct a recipe declaration with an optional check program.
   * \param release Package release coordinates.
   * \param metadata Package metadata.
   * \param sources Declared source inputs.
   * \param build_program Exact build program.
   * \param requirements Direct requirement declarations.
   * \param lifecycle_programs Action-bound lifecycle programs.
   * \param architectures Build and target architecture constraints.
   * \param provenance Exact recipe declaration site.
   * \param check_program Optional exact check program.
   */
  recipe_declaration(package_release release,
                     package_metadata metadata,
                     std::vector<source_input> sources,
                     program build_program,
                     std::vector<requirement_declaration> requirements,
                     std::vector<lifecycle_program> lifecycle_programs,
                     architecture_requirements architectures,
                     declaration_provenance provenance,
                     std::optional<program> check_program);

  /** Return package release coordinates.
   * \return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const package_release& release() const noexcept;
  /** Return package metadata.
   * \return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const package_metadata& metadata() const noexcept;
  /** Return declared source inputs.
   * \return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const std::vector<source_input>& sources() const noexcept;
  /** Return the exact build program.
   * \return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const program& build_program() const noexcept;
  /** Return the optional exact check program.
   * \return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const std::optional<program>& check_program() const noexcept;
  /** Return direct requirement declarations.
   * \return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const std::vector<requirement_declaration>&
  requirements() const noexcept;
  /** Return action-bound lifecycle programs.
   * \return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const std::vector<lifecycle_program>&
  lifecycle_programs() const noexcept;
  /** Return architecture constraints.
   * \return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const architecture_requirements& architectures() const noexcept;
  /** Return recipe declaration provenance.
   * \return Reference valid for the lifetime of this declaration.
   */
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

/** \brief Complete normalized recipe authority. */
class PKGSOURCE_API sealed_recipe final {
public:
  /** Construct normalized recipe authority without a check program.
   * \param release Package release coordinates.
   * \param metadata Package metadata.
   * \param sources Canonical source inputs.
   * \param build_program Exact build program.
   * \param requirements Sealed resolved requirement authority.
   * \param lifecycle_programs Canonical action-bound lifecycle programs.
   * \param architectures Build and target architecture constraints.
   * \param provenance Exact recipe declaration site.
   * \throws error when the supplied reconstruction is not canonical or does
   *         not satisfy recipe closure invariants.
   */
  sealed_recipe(package_release release,
                package_metadata metadata,
                std::vector<source_input> sources,
                program build_program,
                sealed_requirement_set requirements,
                std::vector<lifecycle_program> lifecycle_programs,
                architecture_requirements architectures,
                declaration_provenance provenance);

  /** Construct normalized recipe authority with an optional check program.
   * \param release Package release coordinates.
   * \param metadata Package metadata.
   * \param sources Canonical source inputs.
   * \param build_program Exact build program.
   * \param requirements Sealed resolved requirement authority.
   * \param lifecycle_programs Canonical action-bound lifecycle programs.
   * \param architectures Build and target architecture constraints.
   * \param provenance Exact recipe declaration site.
   * \param check_program Optional exact check program.
   * \throws error when the supplied reconstruction is not canonical or does
   *         not satisfy recipe closure invariants.
   */
  sealed_recipe(package_release release,
                package_metadata metadata,
                std::vector<source_input> sources,
                program build_program,
                sealed_requirement_set requirements,
                std::vector<lifecycle_program> lifecycle_programs,
                architecture_requirements architectures,
                declaration_provenance provenance,
                std::optional<program> check_program);

  /** Return package release coordinates.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const package_release& release() const noexcept;
  /** Return package metadata.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const package_metadata& metadata() const noexcept;
  /** Return canonical source inputs.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const std::vector<source_input>& sources() const noexcept;
  /** Return the exact build program.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const program& build_program() const noexcept;
  /** Return the optional exact check program.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const std::optional<program>& check_program() const noexcept;
  /** Return complete resolved requirement authority.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const sealed_requirement_set& requirements() const noexcept;
  /** Return build requirements.
   * \return Copy of build requirements in canonical order.
   */
  [[nodiscard]] std::vector<resolved_requirement> build_requirements() const;
  /** Return runtime requirements.
   * \return Copy of runtime requirements in canonical order.
   */
  [[nodiscard]] std::vector<resolved_requirement> run_requirements() const;
  /** Return check requirements.
   * \return Copy of check requirements in canonical order.
   */
  [[nodiscard]] std::vector<resolved_requirement> check_requirements() const;
  /** Return requirements for one lifecycle action.
   * \param action Exact lifecycle action.
   * \return Copy of matching requirements in canonical order.
   */
  [[nodiscard]] std::vector<resolved_requirement>
  lifecycle_requirements(lifecycle_action action) const;
  /** Return selected build-profile roots.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const std::vector<selected_profile>&
  selected_build_profiles() const noexcept;
  /** Return the exact retained profile closure.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const std::vector<sealed_profile>&
  profile_closure() const noexcept;
  /** Return canonical lifecycle programs.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const std::vector<lifecycle_program>&
  lifecycle_programs() const noexcept;
  /** Find the program for one lifecycle action.
   * \param action Exact lifecycle action.
   * \return Pointer valid for the lifetime of this recipe, or `nullptr` when
   *         no program is declared for \p action.
   */
  [[nodiscard]] const lifecycle_program*
  lifecycle(lifecycle_action action) const noexcept;
  /** Return architecture constraints.
   * \return Reference valid for the lifetime of this recipe.
   */
  [[nodiscard]] const architecture_requirements& architectures() const noexcept;
  /** Return recipe declaration provenance.
   * \return Reference valid for the lifetime of this recipe.
   */
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

/** Validate, expand, normalize, and seal one native recipe.
 * \param declaration Complete parser-neutral recipe declaration.
 * \param profiles Complete sealed profile universe used for expansion.
 * \return Immutable normalized recipe authority.
 * \throws error for duplicate source destinations, duplicate lifecycle
 *         programs, lifecycle requirements without matching programs, check
 *         requirements without a check program, or invalid profile expansion.
 */
[[nodiscard]] PKGSOURCE_API sealed_recipe
seal_recipe(recipe_declaration declaration, const profile_catalog& profiles);

} // namespace pkgsource
