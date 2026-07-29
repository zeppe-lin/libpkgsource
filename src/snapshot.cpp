// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/snapshot.h>

#include <libpkgsource/error.h>

#include "identity_support.h"

#include <utility>

namespace pkgsource {
namespace {

bool line_safe(std::string_view value)
{
  if (value.empty())
    return false;
  for (unsigned char c : value)
    if (c == 0 || c == '\n' || c == '\r' || c < 0x20 || c == 0x7f)
      return false;
  return true;
}

source_snapshot_identity make_snapshot_identity(const sealed_recipe& recipe)
{
  detail::identity_writer writer;
  writer.text("libpkgsource/source-snapshot/v1");
  writer.text(recipe.identity().hex());
  return source_snapshot_identity::from_sha256(writer.finish());
}

} // namespace

std::string_view to_string(source_syntax value) noexcept
{
  switch (value) {
  case source_syntax::recipe_yaml_v1: return "recipe.yml/1";
  case source_syntax::recipe_yaml_v2: return "recipe.yml/2";
  }
  return "unknown";
}

source_origin::source_origin(std::string document)
    : document_(std::move(document))
{
  if (!line_safe(document_))
    throw error(error_code::invalid_request, "invalid source document origin");
}
const std::string& source_origin::document() const noexcept { return document_; }

source_snapshot::source_snapshot(source_origin origin, source_syntax syntax,
                                 sealed_recipe recipe,
                                 source_snapshot_identity identity)
    : origin_(std::move(origin)), syntax_(syntax), recipe_(std::move(recipe)),
      identity_(std::move(identity))
{
}
const source_origin& source_snapshot::origin() const noexcept { return origin_; }
source_syntax source_snapshot::syntax() const noexcept { return syntax_; }
const sealed_recipe& source_snapshot::recipe() const noexcept { return recipe_; }
const source_snapshot_identity& source_snapshot::identity() const noexcept
{
  return identity_;
}

source_snapshot seal_source(source_origin origin, source_syntax syntax,
                            recipe_declaration declaration,
                            const profile_catalog& profiles)
{
  sealed_recipe recipe = seal_recipe(std::move(declaration), profiles);
  if (syntax == source_syntax::recipe_yaml_v1 && recipe.check_program())
    throw error(error_code::invalid_recipe,
                "recipe.yml/1 cannot declare a check program");
  if (syntax == source_syntax::recipe_yaml_v2 &&
      !recipe.check_requirements().empty() && !recipe.check_program())
    throw error(error_code::invalid_recipe,
                "check requirements without check program");
  const source_snapshot_identity identity = make_snapshot_identity(recipe);
  return source_snapshot(std::move(origin), syntax, std::move(recipe), identity);
}

} // namespace pkgsource
