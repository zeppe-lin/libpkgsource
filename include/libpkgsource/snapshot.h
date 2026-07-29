// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file snapshot.h
 *  \brief Immutable native source snapshots.
 */
#pragma once

#include <string>
#include <string_view>

#include <libpkgsource/recipe.h>

namespace pkgsource {

/*! \brief Input syntax recorded for diagnostics, never semantic authority. */
enum class source_syntax { recipe_yaml_v1, recipe_yaml_v2 };
[[nodiscard]] std::string_view to_string(source_syntax value) noexcept;

/*! \brief Stable diagnostic origin of one source document. */
class source_origin final {
public:
  explicit source_origin(std::string document);
  [[nodiscard]] const std::string& document() const noexcept;
private:
  std::string document_;
};

/*! \brief Sealed native source authority and its syntax provenance. */
class source_snapshot final {
public:
  source_snapshot(source_origin origin, source_syntax syntax,
                  sealed_recipe recipe, source_snapshot_identity identity);
  [[nodiscard]] const source_origin& origin() const noexcept;
  [[nodiscard]] source_syntax syntax() const noexcept;
  [[nodiscard]] const sealed_recipe& recipe() const noexcept;
  [[nodiscard]] const source_snapshot_identity& identity() const noexcept;
private:
  source_origin origin_;
  source_syntax syntax_;
  sealed_recipe recipe_;
  source_snapshot_identity identity_;
};

/*! \brief Seal declarations into syntax-independent source authority. */
[[nodiscard]] source_snapshot seal_source(
    source_origin origin, source_syntax syntax,
    recipe_declaration declaration, const profile_catalog& profiles);

} // namespace pkgsource
