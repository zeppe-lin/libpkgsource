// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "value_codec.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pkgsource::codec::internal {

void encode_provenance(record_writer& output,
                       const declaration_provenance& value)
{
  output.text(value.document());
  output.text(value.path());
  output.u32(value.line());
  output.u32(value.column());
}

declaration_provenance decode_provenance(record_reader& input)
{
  auto document = input.text();
  auto path = input.text();
  const auto line = input.u32();
  const auto column = input.u32();
  return declaration_provenance(
      std::move(document), std::move(path), line, column);
}

void encode_subject(record_writer& output, const requirement_subject& value)
{
  switch (value.kind()) {
  case requirement_subject_kind::package:
    output.u8(1U);
    output.text(value.package().name());
    return;
  case requirement_subject_kind::profile:
    output.u8(2U);
    output.text(value.profile().name());
    return;
  }
  fail(codec_error_code::invalid_record,
       "unknown package-source requirement subject");
}

requirement_subject decode_subject(record_reader& input)
{
  const auto kind = input.u8();
  auto value = input.text();
  if (kind == 1U) {
    return requirement_subject(package_reference(std::move(value)));
  }
  if (kind == 2U) {
    return requirement_subject(profile_reference(std::move(value)));
  }
  fail(codec_error_code::invalid_record,
       "invalid package-source requirement subject");
}

void encode_scope(record_writer& output, const requirement_scope& value)
{
  switch (value.kind()) {
  case requirement_scope_kind::build:
    output.u8(1U);
    return;
  case requirement_scope_kind::run:
    output.u8(2U);
    return;
  case requirement_scope_kind::check:
    output.u8(3U);
    return;
  case requirement_scope_kind::lifecycle:
    output.u8(4U);
    break;
  }

  switch (*value.action()) {
  case lifecycle_action::pre_install:
    output.u8(1U);
    return;
  case lifecycle_action::post_install:
    output.u8(2U);
    return;
  case lifecycle_action::pre_remove:
    output.u8(3U);
    return;
  case lifecycle_action::post_remove:
    output.u8(4U);
    return;
  }
  fail(codec_error_code::invalid_record,
       "unknown package-source lifecycle scope");
}

requirement_scope decode_scope(record_reader& input)
{
  switch (input.u8()) {
  case 1U:
    return requirement_scope::build();
  case 2U:
    return requirement_scope::run();
  case 3U:
    return requirement_scope::check();
  case 4U:
    switch (input.u8()) {
    case 1U:
      return requirement_scope::lifecycle(lifecycle_action::pre_install);
    case 2U:
      return requirement_scope::lifecycle(lifecycle_action::post_install);
    case 3U:
      return requirement_scope::lifecycle(lifecycle_action::pre_remove);
    case 4U:
      return requirement_scope::lifecycle(lifecycle_action::post_remove);
    }
    break;
  }
  fail(codec_error_code::invalid_record,
       "invalid package-source requirement scope");
}

void encode_program(record_writer& output, const program& value)
{
  switch (value.language()) {
  case program_language::posix_shell:
    output.u8(1U);
    break;
  }
  output.text(value.material());
}

program decode_program(record_reader& input)
{
  if (input.u8() != 1U) {
    fail(codec_error_code::invalid_record,
         "invalid package-source program language");
  }
  return program(program_language::posix_shell, input.text());
}

void encode_metadata(record_writer& output, const package_metadata& value)
{
  output.text(value.summary());
  output.u8(value.description().has_value() ? 1U : 0U);
  if (value.description()) {
    output.text(*value.description());
  }
  output.u8(value.homepage().has_value() ? 1U : 0U);
  if (value.homepage()) {
    output.text(*value.homepage());
  }
  output.u32(record_count(value.licenses().size()));
  for (const auto& license : value.licenses()) {
    output.text(license);
  }
}

package_metadata decode_metadata(record_reader& input)
{
  auto summary = input.text();
  std::optional<std::string> description;
  const auto description_present = input.u8();
  if (description_present == 1U) {
    description = input.text();
  } else if (description_present != 0U) {
    fail(codec_error_code::invalid_record,
         "invalid package metadata description presence flag");
  }

  std::optional<std::string> homepage;
  const auto homepage_present = input.u8();
  if (homepage_present == 1U) {
    homepage = input.text();
  } else if (homepage_present != 0U) {
    fail(codec_error_code::invalid_record,
         "invalid package metadata homepage presence flag");
  }

  std::vector<std::string> licenses;
  const auto license_count = read_record_count(input);
  licenses.reserve(license_count);
  for (std::uint32_t index = 0; index != license_count; ++index) {
    licenses.push_back(input.text());
  }

  return package_metadata(std::move(summary),
                          std::move(description),
                          std::move(homepage),
                          std::move(licenses));
}

void encode_source_input(record_writer& output, const source_input& value)
{
  output.u8(value.kind() == source_input_kind::remote ? 1U : 2U);
  output.text(value.location());
  output.text(value.local_name());
  output.u8(value.unpack_kind() == source_unpack_kind::none ? 1U : 2U);
  output.u8(1U);
  output.text(value.content_digest().hex());
}

source_input decode_source_input(record_reader& input)
{
  const auto kind = input.u8();
  auto location = input.text();
  auto local_name = input.text();
  const auto unpack_value = input.u8();
  source_unpack_kind unpack;
  if (unpack_value == 1U) {
    unpack = source_unpack_kind::none;
  } else if (unpack_value == 2U) {
    unpack = source_unpack_kind::archive;
  } else {
    fail(codec_error_code::invalid_record, "invalid package-source unpack policy");
  }
  if (input.u8() != 1U) {
    fail(codec_error_code::invalid_record,
         "invalid package-source digest algorithm");
  }
  digest content(digest_algorithm::sha256, input.text());
  if (kind == 1U) {
    return source_input::remote(
        std::move(location), std::move(local_name), std::move(content), unpack);
  }
  if (kind == 2U) {
    return source_input::local(
        std::move(location), std::move(local_name), std::move(content), unpack);
  }
  fail(codec_error_code::invalid_record, "invalid package-source input kind");
}

void encode_lifecycle_program(record_writer& output,
                              const lifecycle_program& value)
{
  switch (value.action()) {
  case lifecycle_action::pre_install:
    output.u8(1U);
    break;
  case lifecycle_action::post_install:
    output.u8(2U);
    break;
  case lifecycle_action::pre_remove:
    output.u8(3U);
    break;
  case lifecycle_action::post_remove:
    output.u8(4U);
    break;
  }
  encode_program(output, value.value());
}

lifecycle_program decode_lifecycle_program(record_reader& input)
{
  lifecycle_action action;
  switch (input.u8()) {
  case 1U:
    action = lifecycle_action::pre_install;
    break;
  case 2U:
    action = lifecycle_action::post_install;
    break;
  case 3U:
    action = lifecycle_action::pre_remove;
    break;
  case 4U:
    action = lifecycle_action::post_remove;
    break;
  default:
    fail(codec_error_code::invalid_record,
         "invalid package-source lifecycle action");
  }
  return lifecycle_program(action, decode_program(input));
}

void encode_architectures(record_writer& output,
                          const architecture_requirements& value)
{
  output.u32(record_count(value.build().size()));
  for (const auto& architecture : value.build()) {
    output.text(architecture.name());
  }
  output.u32(record_count(value.target().size()));
  for (const auto& architecture : value.target()) {
    output.text(architecture.name());
  }
}

architecture_requirements decode_architectures(record_reader& input)
{
  std::vector<architecture_reference> build;
  const auto build_count = read_record_count(input);
  build.reserve(build_count);
  for (std::uint32_t index = 0; index != build_count; ++index) {
    build.emplace_back(input.text());
  }

  std::vector<architecture_reference> target;
  const auto target_count = read_record_count(input);
  target.reserve(target_count);
  for (std::uint32_t index = 0; index != target_count; ++index) {
    target.emplace_back(input.text());
  }
  return architecture_requirements(std::move(build), std::move(target));
}

} // namespace pkgsource::codec::internal
