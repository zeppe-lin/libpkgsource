// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource/error.h>
#include <libpkgsource/snapshot.h>

#include "internal/identity_writer.h"

#include <optional>
#include <string_view>
#include <utility>

namespace pkgsource {
namespace {

bool line_safe(std::string_view value)
{
  if (value.empty()) {
    return false;
  }
  for (unsigned char c : value) {
    if (c == 0 || c == '\n' || c == '\r' || c < 0x20 || c == 0x7f) {
      return false;
    }
  }
  return true;
}

void write_optional(
    detail::identity_writer& writer,
    const std::optional<std::string>& value)
{
  writer.number(value.has_value() ? 1 : 0);
  if (value) {
    writer.text(*value);
  }
}

void write_scope(
    detail::identity_writer& writer,
    const requirement_scope& scope)
{
  writer.text(to_string(scope.kind()));
  if (scope.action()) {
    writer.text(to_string(*scope.action()));
  }
}

source_snapshot_identity make_snapshot_identity(const sealed_recipe& recipe)
{
  detail::identity_writer writer;
  writer.text("libpkgsource/source-snapshot/v1");
  writer.text(recipe.release().identity().hex());
  writer.text(recipe.metadata().summary());
  write_optional(writer, recipe.metadata().description());
  write_optional(writer, recipe.metadata().homepage());
  writer.number(recipe.metadata().licenses().size());
  for (const std::string& license : recipe.metadata().licenses()) {
    writer.text(license);
  }

  writer.number(recipe.sources().size());
  for (const source_input& source : recipe.sources()) {
    writer.text(to_string(source.kind()));
    writer.text(source.location());
    writer.text(source.local_name());
    writer.text(to_string(source.unpack_kind()));
    writer.text(to_string(source.content_digest().algorithm()));
    writer.text(source.content_digest().hex());
  }

  writer.text(to_string(recipe.build_program().language()));
  writer.text(recipe.build_program().material());
  writer.number(recipe.check_program().has_value() ? 1 : 0);
  if (recipe.check_program()) {
    writer.text(to_string(recipe.check_program()->language()));
    writer.text(recipe.check_program()->material());
  }

  writer.number(recipe.requirements().requirements().size());
  for (const resolved_requirement& requirement :
       recipe.requirements().requirements()) {
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

  writer.number(recipe.profile_closure().size());
  for (const sealed_profile& profile : recipe.profile_closure()) {
    writer.text(profile.name().name());
    writer.text(profile.identity().hex());
  }

  writer.number(recipe.lifecycle_programs().size());
  for (const lifecycle_program& lifecycle : recipe.lifecycle_programs()) {
    writer.text(to_string(lifecycle.action()));
    writer.text(to_string(lifecycle.value().language()));
    writer.text(lifecycle.value().material());
  }

  writer.number(recipe.architectures().build().size());
  for (const architecture_reference& architecture :
       recipe.architectures().build()) {
    writer.text(architecture.name());
  }
  writer.number(recipe.architectures().target().size());
  for (const architecture_reference& architecture :
       recipe.architectures().target()) {
    writer.text(architecture.name());
  }

  return source_snapshot_identity::from_sha256(writer.finish());
}

} // namespace

source_origin::source_origin(std::string document)
    : document_(std::move(document))
{
  if (!line_safe(document_)) {
    throw error(error_code::invalid_request, "invalid source document origin");
  }
}
const std::string& source_origin::document() const noexcept
{
  return document_;
}

source_snapshot::source_snapshot(source_origin origin,
                                 sealed_recipe recipe,
                                 source_snapshot_identity identity)
    : origin_(std::move(origin)), recipe_(std::move(recipe)),
      identity_(std::move(identity))
{
  if (make_snapshot_identity(recipe_) != identity_) {
    throw error(error_code::invalid_identity,
                "source snapshot identity does not match its recipe");
  }
}
const source_origin& source_snapshot::origin() const noexcept
{
  return origin_;
}
const sealed_recipe& source_snapshot::recipe() const noexcept
{
  return recipe_;
}
const source_snapshot_identity& source_snapshot::identity() const noexcept
{
  return identity_;
}

source_snapshot seal_source(source_origin origin,
                            recipe_declaration declaration,
                            const profile_catalog& profiles)
{
  sealed_recipe recipe = seal_recipe(std::move(declaration), profiles);
  const source_snapshot_identity identity = make_snapshot_identity(recipe);
  return source_snapshot(std::move(origin), std::move(recipe), identity);
}

} // namespace pkgsource
