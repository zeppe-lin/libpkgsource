// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/** @file error.h
 *  @brief Typed failures reported by the semantic source authority.
 */
#pragma once

#include <libpkgsource/export.h>

#include <stdexcept>
#include <string>

/** @brief Parser-neutral package-source authority. */
namespace pkgsource {

/** @brief Stable failure categories exposed by libpkgsource. */
enum class error_code {
  invalid_request,       ///< A caller supplied an invalid general request.
  invalid_identity,      ///< A name or identity value is not canonical.
  invalid_provenance,    ///< Declaration provenance is incomplete or unsafe.
  invalid_requirement,   ///< A requirement declaration is internally invalid.
  invalid_profile,       ///< A profile declaration or expansion is invalid.
  duplicate_declaration, ///< Canonical normalization found duplicate authority.
  unknown_profile,       ///< A referenced profile is absent from the catalog.
  profile_cycle,         ///< Nested profile expansion contains a cycle.
  invalid_recipe,        ///< Complete recipe invariants are not satisfied.
  invalid_metadata,      ///< Package metadata is empty or unsafe.
  invalid_source,        ///< A source location or local name is invalid.
  invalid_program,       ///< Program language or exact bytes are invalid.
  identity_failed,       ///< Semantic identity construction failed.
};

/** @brief Exception carrying a stable error category and diagnostic text. */
class PKGSOURCE_API error : public std::runtime_error {
public:
  /** Construct a typed semantic-authority failure.
   *
   * @param code Stable machine-readable failure category.
   * @param message Human-readable diagnostic text.
   */
  error(error_code code, std::string message);

  /** Destroy the polymorphic exception value. */
  ~error() override;

  /** Return the stable failure category.
   *
   * @return Category supplied at construction.
   */
  [[nodiscard]] error_code code() const noexcept;

private:
  error_code code_;
};

} // namespace pkgsource
