// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/** @file snapshot.h
 *  @brief Immutable parser-neutral package-source snapshots.
 */
#pragma once

#include <libpkgsource/export.h>
#include <libpkgsource/recipe.h>

#include <string>

namespace pkgsource {

/** @brief Stable diagnostic origin of one source declaration document. */
class PKGSOURCE_API source_origin final {
public:
  /** Construct a source origin.
   * @param document Non-empty single-line diagnostic document identifier.
   * @throws error with error_code::invalid_request when @p document is unsafe.
   */
  explicit source_origin(std::string document);

  /** Return the diagnostic document identifier.
   * @return Reference valid for the lifetime of this origin.
   */
  [[nodiscard]] const std::string& document() const noexcept;

private:
  std::string document_;
};

/** @brief Complete sealed package-source authority. */
class PKGSOURCE_API source_snapshot final {
public:
  /** Construct a sealed source snapshot.
   *
   * This constructor exists for owner reconstruction. Ordinary callers should
   * use seal_source().
   *
   * @param origin Diagnostic source-document origin.
   * @param recipe Complete normalized recipe authority.
   * @param identity Claimed source-snapshot semantic identity.
   * @throws error with error_code::invalid_identity when @p identity does not
   *         match the complete supplied sealed recipe.
   */
  source_snapshot(source_origin origin,
                  sealed_recipe recipe,
                  source_snapshot_identity identity);

  /** Return diagnostic source origin.
   * @return Reference valid for the lifetime of this snapshot.
   */
  [[nodiscard]] const source_origin& origin() const noexcept;

  /** Return complete normalized recipe authority.
   * @return Reference valid for the lifetime of this snapshot.
   */
  [[nodiscard]] const sealed_recipe& recipe() const noexcept;

  /** Return source-snapshot semantic identity.
   * @return Reference valid for the lifetime of this snapshot.
   */
  [[nodiscard]] const source_snapshot_identity& identity() const noexcept;

private:
  source_origin origin_;
  sealed_recipe recipe_;
  source_snapshot_identity identity_;
};

/** Seal one parser-neutral declaration into complete source authority.
 * @param origin Diagnostic source-document origin.
 * @param declaration Complete parser-neutral recipe declaration.
 * @param profiles Complete sealed profile universe used for expansion.
 * @return Immutable source snapshot with a deterministic semantic identity.
 * @throws error propagated from recipe sealing or identity construction.
 */
[[nodiscard]] PKGSOURCE_API source_snapshot
seal_source(source_origin origin,
            recipe_declaration declaration,
            const profile_catalog& profiles);

} // namespace pkgsource
