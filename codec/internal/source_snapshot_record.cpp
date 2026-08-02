// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource-codec/codec.h>

#include "record_io.h"
#include "value_codec.h"

#include <array>
#include <exception>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace pkgsource::codec {
namespace {

constexpr std::array<std::uint8_t, 8> source_snapshot_magic = {
    'Z',
    'L',
    'P',
    'S',
    'S',
    'N',
    'A',
    'P',
};

[[nodiscard]] profile_catalog
catalog_from_profiles(const std::vector<sealed_profile>& profiles)
{
  std::vector<profile_declaration> declarations;
  declarations.reserve(profiles.size());
  for (const auto& profile : profiles) {
    declarations.emplace_back(
        profile.name(), profile.provenance(), profile.direct_members());
  }
  return profile_catalog::seal(std::move(declarations));
}

[[nodiscard]] std::vector<requirement_declaration>
declarations_from_recipe(const sealed_recipe& recipe)
{
  using declaration_key = std::pair<requirement_scope, requirement_subject>;
  std::map<declaration_key, declaration_provenance> declarations;

  for (const auto& requirement : recipe.requirements().requirements()) {
    for (const auto& origin : requirement.origins()) {
      requirement_subject subject =
          origin.expansion().empty()
              ? requirement_subject(requirement.package())
              : requirement_subject(origin.expansion().front().profile());
      declaration_key key(requirement.scope(), subject);
      const auto inserted =
          declarations.emplace(std::move(key), origin.declaration());
      if (!inserted.second && inserted.first->second != origin.declaration()) {
        internal::fail(
            codec_error_code::invalid_record,
            "sealed requirement has contradictory declaration provenance");
      }
    }
  }

  std::vector<requirement_declaration> result;
  result.reserve(declarations.size());
  for (const auto& entry : declarations) {
    result.emplace_back(entry.first.first, entry.first.second, entry.second);
  }
  return result;
}

} // namespace

source_snapshot_encoding encode_source_snapshot(const source_snapshot& snapshot)
{
  const auto catalog =
      catalog_from_profiles(snapshot.recipe().profile_closure());
  const auto catalog_encoding = encode_profile_catalog(catalog);
  const auto requirements = declarations_from_recipe(snapshot.recipe());

  internal::record_writer output(maximum_source_snapshot_encoding_size);
  output.raw(source_snapshot_magic);
  output.u16(source_snapshot_encoding_version);
  output.text(snapshot.origin().document());
  output.text(snapshot.identity().hex());
  output.blob(catalog_encoding);

  const auto& release = snapshot.recipe().release();
  output.text(release.package().name());
  output.text(release.version());
  output.u32(release.release());
  internal::encode_metadata(output, snapshot.recipe().metadata());

  output.u32(internal::record_count(snapshot.recipe().sources().size()));
  for (const auto& source : snapshot.recipe().sources()) {
    internal::encode_source_input(output, source);
  }
  internal::encode_program(output, snapshot.recipe().build_program());
  output.u8(snapshot.recipe().check_program().has_value() ? 1U : 0U);
  if (snapshot.recipe().check_program()) {
    internal::encode_program(output, *snapshot.recipe().check_program());
  }

  output.u32(internal::record_count(requirements.size()));
  for (const auto& requirement : requirements) {
    internal::encode_scope(output, requirement.scope());
    internal::encode_subject(output, requirement.subject());
    internal::encode_provenance(output, requirement.provenance());
  }

  output.u32(
      internal::record_count(snapshot.recipe().lifecycle_programs().size()));
  for (const auto& lifecycle : snapshot.recipe().lifecycle_programs()) {
    internal::encode_lifecycle_program(output, lifecycle);
  }
  internal::encode_architectures(output, snapshot.recipe().architectures());
  internal::encode_provenance(output, snapshot.recipe().provenance());
  return output.finish();
}

source_snapshot decode_source_snapshot(const source_snapshot_encoding& encoding)
{
  try {
    internal::record_reader input(encoding,
                                  maximum_source_snapshot_encoding_size);
    input.expect(source_snapshot_magic, "source snapshot");
    if (input.u16() != source_snapshot_encoding_version) {
      internal::fail(codec_error_code::unsupported_version,
                     "unsupported source snapshot encoding version");
    }

    auto origin = input.text();
    auto expected_snapshot = input.text();
    auto catalog_bytes = input.blob(maximum_profile_catalog_encoding_size);
    auto catalog = decode_profile_catalog(catalog_bytes);

    auto package = input.text();
    auto version = input.text();
    const auto release_number = input.u32();
    auto metadata = internal::decode_metadata(input);

    std::vector<source_input> sources;
    const auto source_count = internal::read_record_count(input);
    sources.reserve(source_count);
    for (std::uint32_t index = 0; index != source_count; ++index) {
      sources.push_back(internal::decode_source_input(input));
    }

    auto build_program = internal::decode_program(input);
    std::optional<program> check_program;
    const auto check_present = input.u8();
    if (check_present == 1U) {
      check_program = internal::decode_program(input);
    } else if (check_present != 0U) {
      internal::fail(codec_error_code::invalid_record,
                     "invalid source snapshot check-program presence flag");
    }

    std::vector<requirement_declaration> requirements;
    const auto requirement_count = internal::read_record_count(input);
    requirements.reserve(requirement_count);
    for (std::uint32_t index = 0; index != requirement_count; ++index) {
      auto scope = internal::decode_scope(input);
      auto subject = internal::decode_subject(input);
      auto provenance = internal::decode_provenance(input);
      requirements.emplace_back(
          std::move(scope), std::move(subject), std::move(provenance));
    }

    std::vector<lifecycle_program> lifecycle;
    const auto lifecycle_count = internal::read_record_count(input);
    lifecycle.reserve(lifecycle_count);
    for (std::uint32_t index = 0; index != lifecycle_count; ++index) {
      lifecycle.push_back(internal::decode_lifecycle_program(input));
    }
    auto architectures = internal::decode_architectures(input);
    auto provenance = internal::decode_provenance(input);
    input.finish();

    package_release release(package_reference(std::move(package)),
                            std::move(version),
                            release_number);
    recipe_declaration declaration =
        check_program ? recipe_declaration(std::move(release),
                                           std::move(metadata),
                                           std::move(sources),
                                           std::move(build_program),
                                           std::move(requirements),
                                           std::move(lifecycle),
                                           std::move(architectures),
                                           std::move(provenance),
                                           std::move(check_program))
                      : recipe_declaration(std::move(release),
                                           std::move(metadata),
                                           std::move(sources),
                                           std::move(build_program),
                                           std::move(requirements),
                                           std::move(lifecycle),
                                           std::move(architectures),
                                           std::move(provenance));

    auto result = seal_source(
        source_origin(std::move(origin)), std::move(declaration), catalog);
    if (result.identity().hex() != expected_snapshot) {
      internal::fail(codec_error_code::identity_mismatch,
                     "source snapshot identity changed during decoding");
    }
    if (encode_source_snapshot(result) != encoding) {
      internal::fail(codec_error_code::noncanonical,
                     "source snapshot record is not canonical");
    }
    return result;
  } catch (const codec_error&) {
    throw;
  } catch (const error& failure) {
    internal::translate_model_failure(failure);
  } catch (const std::exception& failure) {
    internal::fail(codec_error_code::invalid_record,
                   std::string("invalid source snapshot record: ") +
                       failure.what());
  }
}

} // namespace pkgsource::codec
