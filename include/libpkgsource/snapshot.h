// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file snapshot.h
 *  \brief Immutable native source snapshots.
 */
#pragma once

#include <libpkgsource/recipe.h>

#include <string>

namespace pkgsource {

/*! \brief Stable diagnostic origin of one source declaration document. */
class source_origin final {
public:
  explicit source_origin(std::string document);
  [[nodiscard]] const std::string& document() const noexcept;

private:
  std::string document_;
};

/*! \brief Sealed parser-neutral package-source authority. */
class source_snapshot final {
public:
  source_snapshot(source_origin origin,
                  sealed_recipe recipe,
                  source_snapshot_identity identity);
  [[nodiscard]] const source_origin& origin() const noexcept;
  [[nodiscard]] const sealed_recipe& recipe() const noexcept;
  [[nodiscard]] const source_snapshot_identity& identity() const noexcept;

private:
  source_origin origin_;
  sealed_recipe recipe_;
  source_snapshot_identity identity_;
};

/*! \brief Seal one parser-neutral declaration into source authority. */
[[nodiscard]] source_snapshot seal_source(source_origin origin,
                                          recipe_declaration declaration,
                                          const profile_catalog& profiles);

} // namespace pkgsource
