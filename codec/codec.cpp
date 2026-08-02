// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource-codec/codec.h>

#include <libpkgsource/error.h>

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "internal/record_checksum.h"
#include <utility>
#include <vector>

namespace pkgsource::codec {
namespace {

constexpr std::array<std::uint8_t, 8> profile_magic = {
    'Z', 'L', 'P', 'S', 'P', 'C', 'A', 'T',
};
constexpr std::array<std::uint8_t, 8> snapshot_magic = {
    'Z', 'L', 'P', 'S', 'S', 'N', 'A', 'P',
};
constexpr std::size_t checksum_size = 32U;

[[noreturn]] void fail(codec_error_code code, std::string message)
{
  throw codec_error(code, std::move(message));
}

class writer final {
public:
  explicit writer(std::size_t maximum) : maximum_(maximum) {}

  void u8(std::uint8_t value)
  {
    bytes_.push_back(value);
    check_size();
  }

  void u16(std::uint16_t value)
  {
    u8(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    u8(static_cast<std::uint8_t>(value & 0xffU));
  }

  void u32(std::uint32_t value)
  {
    for (int shift = 24; shift >= 0; shift -= 8)
      u8(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }

  void u64(std::uint64_t value)
  {
    for (int shift = 56; shift >= 0; shift -= 8)
      u8(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }

  template <std::size_t N>
  void raw(const std::array<std::uint8_t, N>& value)
  {
    bytes_.insert(bytes_.end(), value.begin(), value.end());
    check_size();
  }

  void raw(const std::vector<std::uint8_t>& value)
  {
    bytes_.insert(bytes_.end(), value.begin(), value.end());
    check_size();
  }

  void text(std::string_view value)
  {
    if (value.size() > std::numeric_limits<std::uint32_t>::max())
      fail(codec_error_code::size_limit,
           "package-source record text exceeds encoding limit");
    u32(static_cast<std::uint32_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
    check_size();
  }

  void blob(const std::vector<std::uint8_t>& value)
  {
    u64(static_cast<std::uint64_t>(value.size()));
    raw(value);
  }

  std::vector<std::uint8_t> finish()
  {
    const auto digest = internal::record_checksum(bytes_.data(), bytes_.size());
    raw(digest);
    return std::move(bytes_);
  }

private:
  void check_size() const
  {
    if (bytes_.size() > maximum_ - checksum_size)
      fail(codec_error_code::size_limit,
           "package-source record exceeds encoding limit");
  }

  std::size_t maximum_;
  std::vector<std::uint8_t> bytes_;
};

class reader final {
public:
  reader(const std::vector<std::uint8_t>& bytes, std::size_t maximum)
      : bytes_(bytes), limit_(bytes.size() >= checksum_size
                                 ? bytes.size() - checksum_size
                                 : 0U)
  {
    if (bytes.size() > maximum)
      fail(codec_error_code::size_limit,
           "package-source record exceeds decoding limit");
    if (bytes.size() < checksum_size)
      fail(codec_error_code::truncated,
           "package-source record is shorter than its checksum");
    const auto actual = internal::record_checksum(bytes.data(), limit_);
    if (!std::equal(actual.begin(), actual.end(), bytes.begin() + limit_))
      fail(codec_error_code::checksum_mismatch,
           "package-source record checksum mismatch");
  }

  std::uint8_t u8()
  {
    require(1U);
    return bytes_[offset_++];
  }

  std::uint16_t u16()
  {
    return static_cast<std::uint16_t>(u8()) << 8U |
           static_cast<std::uint16_t>(u8());
  }

  std::uint32_t u32()
  {
    std::uint32_t value = 0;
    for (int i = 0; i != 4; ++i)
      value = (value << 8U) | u8();
    return value;
  }

  std::uint64_t u64()
  {
    std::uint64_t value = 0;
    for (int i = 0; i != 8; ++i)
      value = (value << 8U) | u8();
    return value;
  }

  template <std::size_t N>
  void expect(const std::array<std::uint8_t, N>& expected,
              std::string_view name)
  {
    require(N);
    if (!std::equal(expected.begin(), expected.end(), bytes_.begin() + offset_))
      fail(codec_error_code::invalid_magic,
           "invalid " + std::string(name) + " magic");
    offset_ += N;
  }

  std::string text()
  {
    const auto size = u32();
    require(size);
    std::string result(
        reinterpret_cast<const char*>(bytes_.data() + offset_), size);
    offset_ += size;
    return result;
  }

  std::vector<std::uint8_t> blob(std::size_t maximum)
  {
    const auto size = u64();
    if (size > maximum || size > std::numeric_limits<std::size_t>::max())
      fail(codec_error_code::size_limit,
           "embedded package-source record exceeds decoding limit");
    require(static_cast<std::size_t>(size));
    std::vector<std::uint8_t> result(
        bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
        bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
    offset_ += static_cast<std::size_t>(size);
    return result;
  }

  void finish() const
  {
    if (offset_ != limit_)
      fail(codec_error_code::invalid_record,
           "package-source record has trailing fields");
  }

private:
  void require(std::size_t size) const
  {
    if (offset_ > limit_ || size > limit_ - offset_)
      fail(codec_error_code::truncated,
           "package-source record is truncated");
  }

  const std::vector<std::uint8_t>& bytes_;
  std::size_t limit_;
  std::size_t offset_ = 0;
};

std::uint32_t count(std::size_t value)
{
  if (value > maximum_record_item_count)
    fail(codec_error_code::size_limit,
         "package-source collection exceeds item limit");
  return static_cast<std::uint32_t>(value);
}

std::uint32_t read_count(reader& input)
{
  const auto value = input.u32();
  if (value > maximum_record_item_count)
    fail(codec_error_code::size_limit,
         "package-source collection exceeds item limit");
  return value;
}

void encode_provenance(writer& output, const declaration_provenance& value)
{
  output.text(value.document());
  output.text(value.path());
  output.u32(value.line());
  output.u32(value.column());
}

declaration_provenance decode_provenance(reader& input)
{
  auto document = input.text();
  auto path = input.text();
  const auto line = input.u32();
  const auto column = input.u32();
  return declaration_provenance(
      std::move(document), std::move(path), line, column);
}

void encode_subject(writer& output, const requirement_subject& value)
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

requirement_subject decode_subject(reader& input)
{
  const auto kind = input.u8();
  auto value = input.text();
  if (kind == 1U)
    return requirement_subject(package_reference(std::move(value)));
  if (kind == 2U)
    return requirement_subject(profile_reference(std::move(value)));
  fail(codec_error_code::invalid_record,
       "invalid package-source requirement subject");
}

void encode_scope(writer& output, const requirement_scope& value)
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
    case lifecycle_action::pre_install: output.u8(1U); return;
    case lifecycle_action::post_install: output.u8(2U); return;
    case lifecycle_action::pre_remove: output.u8(3U); return;
    case lifecycle_action::post_remove: output.u8(4U); return;
  }
  fail(codec_error_code::invalid_record,
       "unknown package-source lifecycle scope");
}

requirement_scope decode_scope(reader& input)
{
  switch (input.u8()) {
    case 1U: return requirement_scope::build();
    case 2U: return requirement_scope::run();
    case 3U: return requirement_scope::check();
    case 4U:
      switch (input.u8()) {
        case 1U: return requirement_scope::lifecycle(
            lifecycle_action::pre_install);
        case 2U: return requirement_scope::lifecycle(
            lifecycle_action::post_install);
        case 3U: return requirement_scope::lifecycle(
            lifecycle_action::pre_remove);
        case 4U: return requirement_scope::lifecycle(
            lifecycle_action::post_remove);
      }
      break;
  }
  fail(codec_error_code::invalid_record,
       "invalid package-source requirement scope");
}

void encode_program(writer& output, const program& value)
{
  switch (value.language()) {
    case program_language::posix_shell: output.u8(1U); break;
  }
  output.text(value.material());
}

program decode_program(reader& input)
{
  if (input.u8() != 1U)
    fail(codec_error_code::invalid_record,
         "invalid package-source program language");
  return program(program_language::posix_shell, input.text());
}

void encode_profile_declaration(writer& output, const sealed_profile& profile)
{
  output.text(profile.name().name());
  output.text(profile.identity().hex());
  encode_provenance(output, profile.provenance());
  output.u32(count(profile.direct_members().size()));
  for (const auto& member : profile.direct_members()) {
    encode_subject(output, member.subject());
    encode_provenance(output, member.provenance());
  }
}

struct decoded_profile final {
  profile_declaration declaration;
  std::string identity;
};

decoded_profile decode_profile_declaration(reader& input)
{
  auto name = input.text();
  auto identity = input.text();
  auto provenance = decode_provenance(input);
  std::vector<profile_member_declaration> members;
  const auto member_count = read_count(input);
  members.reserve(member_count);
  for (std::uint32_t i = 0; i != member_count; ++i) {
    auto subject = decode_subject(input);
    auto member_provenance = decode_provenance(input);
    members.emplace_back(
        std::move(subject), std::move(member_provenance));
  }
  return decoded_profile{
      profile_declaration(profile_reference(std::move(name)),
                          std::move(provenance), std::move(members)),
      std::move(identity)};
}

profile_catalog catalog_from_profiles(
    const std::vector<sealed_profile>& profiles)
{
  std::vector<profile_declaration> declarations;
  declarations.reserve(profiles.size());
  for (const auto& profile : profiles)
    declarations.emplace_back(
        profile.name(), profile.provenance(), profile.direct_members());
  return profile_catalog::seal(std::move(declarations));
}

std::vector<requirement_declaration> declarations_from_recipe(
    const sealed_recipe& recipe)
{
  using key = std::pair<requirement_scope, requirement_subject>;
  std::map<key, declaration_provenance> declarations;
  for (const auto& requirement : recipe.requirements().requirements()) {
    for (const auto& origin : requirement.origins()) {
      requirement_subject subject = origin.expansion().empty()
          ? requirement_subject(requirement.package())
          : requirement_subject(origin.expansion().front().profile());
      key declaration_key(requirement.scope(), subject);
      const auto inserted = declarations.emplace(
          std::move(declaration_key), origin.declaration());
      if (!inserted.second &&
          inserted.first->second != origin.declaration())
        fail(codec_error_code::invalid_record,
             "sealed requirement has contradictory declaration provenance");
    }
  }

  std::vector<requirement_declaration> result;
  result.reserve(declarations.size());
  for (const auto& entry : declarations)
    result.emplace_back(
        entry.first.first, entry.first.second, entry.second);
  return result;
}

void encode_metadata(writer& output, const package_metadata& value)
{
  output.text(value.summary());
  output.u8(value.description().has_value() ? 1U : 0U);
  if (value.description()) output.text(*value.description());
  output.u8(value.homepage().has_value() ? 1U : 0U);
  if (value.homepage()) output.text(*value.homepage());
  output.u32(count(value.licenses().size()));
  for (const auto& license : value.licenses()) output.text(license);
}

package_metadata decode_metadata(reader& input)
{
  auto summary = input.text();
  std::optional<std::string> description;
  const auto description_present = input.u8();
  if (description_present == 1U)
    description = input.text();
  else if (description_present != 0U)
    fail(codec_error_code::invalid_record,
         "invalid package metadata description presence flag");
  std::optional<std::string> homepage;
  const auto homepage_present = input.u8();
  if (homepage_present == 1U)
    homepage = input.text();
  else if (homepage_present != 0U)
    fail(codec_error_code::invalid_record,
         "invalid package metadata homepage presence flag");
  std::vector<std::string> licenses;
  const auto license_count = read_count(input);
  licenses.reserve(license_count);
  for (std::uint32_t i = 0; i != license_count; ++i)
    licenses.push_back(input.text());
  return package_metadata(
      std::move(summary), std::move(description), std::move(homepage),
      std::move(licenses));
}

void encode_source_input(writer& output, const source_input& value)
{
  output.u8(value.kind() == source_input_kind::remote ? 1U : 2U);
  output.text(value.location());
  output.text(value.local_name());
  output.u8(1U);
  output.text(value.content_digest().hex());
}

source_input decode_source_input(reader& input)
{
  const auto kind = input.u8();
  auto location = input.text();
  auto local_name = input.text();
  if (input.u8() != 1U)
    fail(codec_error_code::invalid_record,
         "invalid package-source digest algorithm");
  digest content(digest_algorithm::sha256, input.text());
  if (kind == 1U)
    return source_input::remote(
        std::move(location), std::move(local_name), std::move(content));
  if (kind == 2U)
    return source_input::local(
        std::move(location), std::move(local_name), std::move(content));
  fail(codec_error_code::invalid_record,
       "invalid package-source input kind");
}

void encode_lifecycle_program(writer& output, const lifecycle_program& value)
{
  switch (value.action()) {
    case lifecycle_action::pre_install: output.u8(1U); break;
    case lifecycle_action::post_install: output.u8(2U); break;
    case lifecycle_action::pre_remove: output.u8(3U); break;
    case lifecycle_action::post_remove: output.u8(4U); break;
  }
  encode_program(output, value.value());
}

lifecycle_program decode_lifecycle_program(reader& input)
{
  lifecycle_action action;
  switch (input.u8()) {
    case 1U: action = lifecycle_action::pre_install; break;
    case 2U: action = lifecycle_action::post_install; break;
    case 3U: action = lifecycle_action::pre_remove; break;
    case 4U: action = lifecycle_action::post_remove; break;
    default:
      fail(codec_error_code::invalid_record,
           "invalid package-source lifecycle action");
  }
  auto value = decode_program(input);
  return lifecycle_program(action, std::move(value));
}

void encode_architectures(
    writer& output, const architecture_requirements& value)
{
  output.u32(count(value.build().size()));
  for (const auto& architecture : value.build())
    output.text(architecture.name());
  output.u32(count(value.target().size()));
  for (const auto& architecture : value.target())
    output.text(architecture.name());
}

architecture_requirements decode_architectures(reader& input)
{
  std::vector<architecture_reference> build;
  const auto build_count = read_count(input);
  build.reserve(build_count);
  for (std::uint32_t i = 0; i != build_count; ++i)
    build.emplace_back(input.text());
  std::vector<architecture_reference> target;
  const auto target_count = read_count(input);
  target.reserve(target_count);
  for (std::uint32_t i = 0; i != target_count; ++i)
    target.emplace_back(input.text());
  return architecture_requirements(std::move(build), std::move(target));
}

[[noreturn]] void translate_model_failure(const error& failure)
{
  fail(codec_error_code::invalid_record,
       std::string("invalid package-source record: ") + failure.what());
}

} // namespace

codec_error::codec_error(codec_error_code code, std::string message)
    : std::invalid_argument(std::move(message)), code_(code)
{
}

codec_error_code codec_error::code() const noexcept { return code_; }

profile_catalog_encoding encode_profile_catalog(const profile_catalog& catalog)
{
  writer output(maximum_profile_catalog_encoding_size);
  output.raw(profile_magic);
  output.u16(profile_catalog_encoding_version);
  output.u32(count(catalog.profiles().size()));
  for (const auto& profile : catalog.profiles())
    encode_profile_declaration(output, profile);
  return output.finish();
}

profile_catalog decode_profile_catalog(
    const profile_catalog_encoding& encoding)
{
  try {
    reader input(encoding, maximum_profile_catalog_encoding_size);
    input.expect(profile_magic, "profile catalog");
    if (input.u16() != profile_catalog_encoding_version)
      fail(codec_error_code::unsupported_version,
           "unsupported profile catalog encoding version");
    std::vector<decoded_profile> decoded;
    const auto profile_count = read_count(input);
    decoded.reserve(profile_count);
    for (std::uint32_t i = 0; i != profile_count; ++i)
      decoded.push_back(decode_profile_declaration(input));
    input.finish();

    std::vector<profile_declaration> declarations;
    declarations.reserve(decoded.size());
    for (auto& value : decoded)
      declarations.push_back(std::move(value.declaration));
    profile_catalog result = profile_catalog::seal(std::move(declarations));
    if (result.profiles().size() != decoded.size())
      fail(codec_error_code::identity_mismatch,
           "profile catalog shape changed during sealing");
    for (std::size_t i = 0; i != decoded.size(); ++i)
      if (result.profiles()[i].identity().hex() != decoded[i].identity)
        fail(codec_error_code::identity_mismatch,
             "profile identity changed during decoding");
    if (encode_profile_catalog(result) != encoding)
      fail(codec_error_code::noncanonical,
           "profile catalog record is not canonical");
    return result;
  } catch (const codec_error&) {
    throw;
  } catch (const error& failure) {
    translate_model_failure(failure);
  } catch (const std::exception& failure) {
    fail(codec_error_code::invalid_record,
         std::string("invalid profile catalog record: ") + failure.what());
  }
}

source_snapshot_encoding encode_source_snapshot(
    const source_snapshot& snapshot)
{
  const auto catalog = catalog_from_profiles(
      snapshot.recipe().profile_closure());
  const auto catalog_encoding = encode_profile_catalog(catalog);
  const auto requirements = declarations_from_recipe(snapshot.recipe());

  writer output(maximum_source_snapshot_encoding_size);
  output.raw(snapshot_magic);
  output.u16(source_snapshot_encoding_version);
  output.text(snapshot.origin().document());
  output.text(snapshot.identity().hex());
  output.blob(catalog_encoding);

  const auto& release = snapshot.recipe().release();
  output.text(release.package().name());
  output.text(release.version());
  output.u32(release.release());
  encode_metadata(output, snapshot.recipe().metadata());

  output.u32(count(snapshot.recipe().sources().size()));
  for (const auto& source : snapshot.recipe().sources())
    encode_source_input(output, source);
  encode_program(output, snapshot.recipe().build_program());
  output.u8(snapshot.recipe().check_program().has_value() ? 1U : 0U);
  if (snapshot.recipe().check_program())
    encode_program(output, *snapshot.recipe().check_program());

  output.u32(count(requirements.size()));
  for (const auto& requirement : requirements) {
    encode_scope(output, requirement.scope());
    encode_subject(output, requirement.subject());
    encode_provenance(output, requirement.provenance());
  }

  output.u32(count(snapshot.recipe().lifecycle_programs().size()));
  for (const auto& lifecycle : snapshot.recipe().lifecycle_programs())
    encode_lifecycle_program(output, lifecycle);
  encode_architectures(output, snapshot.recipe().architectures());
  encode_provenance(output, snapshot.recipe().provenance());
  return output.finish();
}

source_snapshot decode_source_snapshot(
    const source_snapshot_encoding& encoding)
{
  try {
    reader input(encoding, maximum_source_snapshot_encoding_size);
    input.expect(snapshot_magic, "source snapshot");
    if (input.u16() != source_snapshot_encoding_version)
      fail(codec_error_code::unsupported_version,
           "unsupported source snapshot encoding version");
    auto origin = input.text();
    auto expected_snapshot = input.text();
    auto catalog_bytes = input.blob(maximum_profile_catalog_encoding_size);
    auto catalog = decode_profile_catalog(catalog_bytes);

    auto package = input.text();
    auto version = input.text();
    const auto release_number = input.u32();
    auto metadata = decode_metadata(input);

    std::vector<source_input> sources;
    const auto source_count = read_count(input);
    sources.reserve(source_count);
    for (std::uint32_t i = 0; i != source_count; ++i)
      sources.push_back(decode_source_input(input));
    auto build_program = decode_program(input);
    std::optional<program> check_program;
    const auto check_present = input.u8();
    if (check_present == 1U)
      check_program = decode_program(input);
    else if (check_present != 0U)
      fail(codec_error_code::invalid_record,
           "invalid source snapshot check-program presence flag");

    std::vector<requirement_declaration> requirements;
    const auto requirement_count = read_count(input);
    requirements.reserve(requirement_count);
    for (std::uint32_t i = 0; i != requirement_count; ++i) {
      auto scope = decode_scope(input);
      auto subject = decode_subject(input);
      auto provenance = decode_provenance(input);
      requirements.emplace_back(
          std::move(scope), std::move(subject), std::move(provenance));
    }

    std::vector<lifecycle_program> lifecycle;
    const auto lifecycle_count = read_count(input);
    lifecycle.reserve(lifecycle_count);
    for (std::uint32_t i = 0; i != lifecycle_count; ++i)
      lifecycle.push_back(decode_lifecycle_program(input));
    auto architectures = decode_architectures(input);
    auto provenance = decode_provenance(input);
    input.finish();

    package_release release(
        package_reference(std::move(package)), std::move(version),
        release_number);
    recipe_declaration declaration = check_program
        ? recipe_declaration(
              std::move(release), std::move(metadata), std::move(sources),
              std::move(build_program), std::move(requirements),
              std::move(lifecycle), std::move(architectures),
              std::move(provenance), std::move(check_program))
        : recipe_declaration(
              std::move(release), std::move(metadata), std::move(sources),
              std::move(build_program), std::move(requirements),
              std::move(lifecycle), std::move(architectures),
              std::move(provenance));
    auto result = seal_source(
        source_origin(std::move(origin)), std::move(declaration), catalog);
    if (result.identity().hex() != expected_snapshot)
      fail(codec_error_code::identity_mismatch,
           "source snapshot identity changed during decoding");
    if (encode_source_snapshot(result) != encoding)
      fail(codec_error_code::noncanonical,
           "source snapshot record is not canonical");
    return result;
  } catch (const codec_error&) {
    throw;
  } catch (const error& failure) {
    translate_model_failure(failure);
  } catch (const std::exception& failure) {
    fail(codec_error_code::invalid_record,
         std::string("invalid source snapshot record: ") + failure.what());
  }
}

} // namespace pkgsource::codec
