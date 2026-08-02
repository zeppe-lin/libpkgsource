// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource-codec/codec.h>
#include <libpkgsource/libpkgsource.h>

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace pkgsource;
using namespace pkgsource::codec;

namespace {

declaration_provenance
at(const char* document, const char* path, std::uint32_t line)
{
  return declaration_provenance(document, path, line, 3);
}

profile_catalog profiles()
{
  return profile_catalog::seal({
      profile_declaration(profile_reference("@compiler"),
                          at("profiles.yml", "compiler", 1),
                          {profile_member_declaration(
                              requirement_subject(package_reference("gcc")),
                              at("profiles.yml", "compiler[0]", 2))}),
      profile_declaration(
          profile_reference("@toolchain"),
          at("profiles.yml", "toolchain", 4),
          {
              profile_member_declaration(
                  requirement_subject(package_reference("binutils")),
                  at("profiles.yml", "toolchain[0]", 5)),
              profile_member_declaration(
                  requirement_subject(profile_reference("@compiler")),
                  at("profiles.yml", "toolchain[1]", 6)),
          }),
      profile_declaration(profile_reference("@runtime"),
                          at("profiles.yml", "runtime", 8),
                          {profile_member_declaration(
                              requirement_subject(package_reference("libfoo")),
                              at("profiles.yml", "runtime[0]", 9))}),
  });
}

recipe_declaration declaration(bool with_check)
{
  std::vector<requirement_declaration> requirements{
      requirement_declaration(
          requirement_scope::build(),
          requirement_subject(profile_reference("@toolchain")),
          at("recipe.yml", "requirements.build[0]", 12)),
      requirement_declaration(
          requirement_scope::run(),
          requirement_subject(profile_reference("@runtime")),
          at("recipe.yml", "requirements.run[0]", 13)),
      requirement_declaration(requirement_scope::run(),
                              requirement_subject(package_reference("libfoo")),
                              at("recipe.yml", "requirements.run[1]", 14)),
      requirement_declaration(
          requirement_scope::lifecycle(lifecycle_action::post_install),
          requirement_subject(package_reference("desktop-file-utils")),
          at("recipe.yml", "requirements.lifecycle.post-install[0]", 16)),
  };

  const std::string a(64, 'a');
  const std::string b(64, 'b');
  package_release release(package_reference("example"), "1.2.3", 4);
  package_metadata metadata("Example package",
                            "Long description",
                            "https://example.invalid",
                            {"MIT", "BSD-2-Clause"});
  std::vector<source_input> sources{
      source_input::remote("https://example.invalid/example.tar.xz",
                           "example.tar.xz",
                           digest(digest_algorithm::sha256, a)),
      source_input::local("files/example.conf",
                          "example.conf",
                          digest(digest_algorithm::sha256, b)),
  };
  program build(program_language::posix_shell, "echo build\n");
  std::vector<lifecycle_program> lifecycle{
      lifecycle_program(
          lifecycle_action::post_install,
          program(program_language::posix_shell, "update-desktop-database\n")),
  };
  architecture_requirements architectures({architecture_reference("x86_64")},
                                          {architecture_reference("x86_64")});
  auto provenance = at("recipe.yml", "$", 1);

  if (with_check) {
    requirements.emplace_back(
        requirement_scope::check(),
        requirement_subject(package_reference("pkgcheck")),
        at("recipe.yml", "requirements.check[0]", 15));
    return recipe_declaration(
        std::move(release),
        std::move(metadata),
        std::move(sources),
        std::move(build),
        std::move(requirements),
        std::move(lifecycle),
        std::move(architectures),
        std::move(provenance),
        program(program_language::posix_shell, "meson test -C build\n"));
  }
  return recipe_declaration(std::move(release),
                            std::move(metadata),
                            std::move(sources),
                            std::move(build),
                            std::move(requirements),
                            std::move(lifecycle),
                            std::move(architectures),
                            std::move(provenance));
}

recipe_declaration minimal_declaration()
{
  return recipe_declaration(
      package_release(package_reference("a"), "1", 1),
      package_metadata("A", std::nullopt, std::nullopt, {"MIT"}),
      {},
      program(program_language::posix_shell, "true\n"),
      {},
      {},
      architecture_requirements({}, {}),
      declaration_provenance("recipe.yml", "document", 1, 1));
}

template <typename Function>
void expect(codec_error_code code, Function&& function)
{
  try {
    function();
    assert(false);
  } catch (const codec_error& failure) {
    assert(failure.code() == code);
  }
}

std::array<std::uint8_t, 32> checksum(const std::vector<std::uint8_t>& value,
                                      std::size_t size)
{
  using context_pointer =
      std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  context_pointer context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  std::array<std::uint8_t, 32> result{};
  unsigned int result_size = 0;
  assert(context);
  assert(EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) == 1);
  assert(EVP_DigestUpdate(context.get(), value.data(), size) == 1);
  assert(EVP_DigestFinal_ex(context.get(), result.data(), &result_size) == 1);
  assert(result_size == result.size());
  return result;
}

std::string sha256_hex(const std::vector<std::uint8_t>& value)
{
  const auto digest = checksum(value, value.size());
  std::ostringstream output;
  output << std::hex << std::setfill('0');
  for (const auto byte : digest) {
    output << std::setw(2) << static_cast<unsigned int>(byte);
  }
  return output.str();
}

void reseal_checksum(std::vector<std::uint8_t>& value)
{
  assert(value.size() >= 32);
  const auto digest = checksum(value, value.size() - 32);
  std::copy(digest.begin(), digest.end(), value.end() - 32);
}

std::uint32_t u32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
  assert(offset + 4 <= bytes.size());
  return static_cast<std::uint32_t>(bytes[offset]) << 24U |
         static_cast<std::uint32_t>(bytes[offset + 1]) << 16U |
         static_cast<std::uint32_t>(bytes[offset + 2]) << 8U |
         static_cast<std::uint32_t>(bytes[offset + 3]);
}

std::uint64_t u64(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
  assert(offset + 8 <= bytes.size());
  std::uint64_t value = 0;
  for (std::size_t i = 0; i != 8; ++i) {
    value = (value << 8U) | bytes[offset + i];
  }
  return value;
}

void set_u32(std::vector<std::uint8_t>& bytes,
             std::size_t offset,
             std::uint32_t value)
{
  assert(offset + 4 <= bytes.size());
  bytes[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xffU);
  bytes[offset + 1] = static_cast<std::uint8_t>((value >> 16U) & 0xffU);
  bytes[offset + 2] = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
  bytes[offset + 3] = static_cast<std::uint8_t>(value & 0xffU);
}

void skip_text(const std::vector<std::uint8_t>& bytes, std::size_t& offset)
{
  const auto size = u32(bytes, offset);
  offset += 4;
  assert(offset + size <= bytes.size());
  offset += size;
}

void skip_source_input(const std::vector<std::uint8_t>& bytes,
                       std::size_t& offset)
{
  assert(offset < bytes.size());
  ++offset;
  skip_text(bytes, offset);
  skip_text(bytes, offset);
  assert(offset < bytes.size());
  ++offset;
  skip_text(bytes, offset);
}

struct source_layout final {
  std::size_t profile_blob = 0;
  std::size_t profile_blob_size = 0;
  std::size_t description_flag = 0;
  std::size_t source_count = 0;
  std::vector<std::pair<std::size_t, std::size_t>> sources;
};

source_layout inspect_source_record(const std::vector<std::uint8_t>& bytes)
{
  source_layout result;
  std::size_t offset = 10;
  skip_text(bytes, offset); // origin
  skip_text(bytes, offset); // stored source identity
  result.profile_blob_size = static_cast<std::size_t>(u64(bytes, offset));
  offset += 8;
  result.profile_blob = offset;
  assert(offset + result.profile_blob_size <= bytes.size());
  offset += result.profile_blob_size;
  skip_text(bytes, offset); // package
  skip_text(bytes, offset); // version
  offset += 4;              // release
  skip_text(bytes, offset); // summary
  result.description_flag = offset;
  const auto description_present = bytes[offset++];
  if (description_present == 1U) {
    skip_text(bytes, offset);
  }
  const auto homepage_present = bytes[offset++];
  if (homepage_present == 1U) {
    skip_text(bytes, offset);
  }
  const auto license_count = u32(bytes, offset);
  offset += 4;
  for (std::uint32_t i = 0; i != license_count; ++i) {
    skip_text(bytes, offset);
  }
  result.source_count = offset;
  const auto count = u32(bytes, offset);
  offset += 4;
  result.sources.reserve(count);
  for (std::uint32_t i = 0; i != count; ++i) {
    const auto begin = offset;
    skip_source_input(bytes, offset);
    result.sources.emplace_back(begin, offset);
  }
  return result;
}

void test_golden_vectors()
{
  const auto empty_profiles = profile_catalog::seal({});
  const auto profile_encoding = encode_profile_catalog(empty_profiles);
  assert(profile_encoding.size() == 46);
  assert(sha256_hex(profile_encoding) ==
         "2f268947090f17e2c4f1825c0c7167930c8950327c6389c6531d8e6f64b4e483");

  const auto snapshot = seal_source(
      source_origin("recipe.yml"), minimal_declaration(), empty_profiles);
  assert(snapshot.identity().hex() ==
         "485b072c47308c1c64e8ec9d8c88c2418c57642c2b50e4e59c853c509d5da838");
  const auto source_encoding = encode_source_snapshot(snapshot);
  assert(source_encoding.size() == 275);
  assert(sha256_hex(source_encoding) ==
         "cd221e9527162de41fa23806f2a370e161139cf059c6dc77d08cbfd37b45be35");
}

void test_profile_catalog_round_trip()
{
  const auto catalog = profiles();
  const auto encoding = encode_profile_catalog(catalog);
  assert(encoding.size() > 42);
  assert(std::equal(
      encoding.begin(),
      encoding.begin() + 8,
      std::array<std::uint8_t, 8>{'Z', 'L', 'P', 'S', 'P', 'C', 'A', 'T'}
          .begin()));
  const auto decoded = decode_profile_catalog(encoding);
  assert(decoded.profiles().size() == 3);
  assert(decoded.require(profile_reference("@toolchain")).expansion().size() ==
         2);
  assert(decoded.require(profile_reference("@toolchain")).identity() ==
         catalog.require(profile_reference("@toolchain")).identity());
  assert(encode_profile_catalog(decoded) == encoding);

  auto reordered = profile_catalog::seal({
      profile_declaration(profile_reference("@runtime"),
                          at("profiles.yml", "runtime", 8),
                          {profile_member_declaration(
                              requirement_subject(package_reference("libfoo")),
                              at("profiles.yml", "runtime[0]", 9))}),
      profile_declaration(
          profile_reference("@toolchain"),
          at("profiles.yml", "toolchain", 4),
          {
              profile_member_declaration(
                  requirement_subject(profile_reference("@compiler")),
                  at("profiles.yml", "toolchain[1]", 6)),
              profile_member_declaration(
                  requirement_subject(package_reference("binutils")),
                  at("profiles.yml", "toolchain[0]", 5)),
          }),
      profile_declaration(profile_reference("@compiler"),
                          at("profiles.yml", "compiler", 1),
                          {profile_member_declaration(
                              requirement_subject(package_reference("gcc")),
                              at("profiles.yml", "compiler[0]", 2))}),
  });
  assert(encode_profile_catalog(reordered) == encoding);
}

void test_source_snapshot_round_trip()
{
  const auto catalog = profiles();
  const auto plain = seal_source(
      source_origin("recipes/example/recipe.yml"), declaration(false), catalog);
  const auto plain_encoding = encode_source_snapshot(plain);
  const auto plain_decoded = decode_source_snapshot(plain_encoding);
  assert(plain_decoded.identity() == plain.identity());
  assert(plain_decoded.origin().document() == plain.origin().document());
  assert(plain_decoded.recipe().build_requirements().size() == 2);
  assert(plain_decoded.recipe().run_requirements().size() == 1);
  assert(plain_decoded.recipe().profile_closure().size() == 3);
  assert(encode_source_snapshot(plain_decoded) == plain_encoding);

  const auto checked = seal_source(
      source_origin("recipes/example/recipe.yml"), declaration(true), catalog);
  const auto checked_encoding = encode_source_snapshot(checked);
  const auto checked_decoded = decode_source_snapshot(checked_encoding);
  assert(checked_decoded.identity() == checked.identity());
  assert(checked_decoded.recipe().check_program());
  assert(checked_decoded.recipe().check_program()->material() ==
         "meson test -C build\n");
  assert(encode_source_snapshot(checked_decoded) == checked_encoding);
}

void test_envelope_refusals()
{
  const auto catalog = profiles();
  const auto snapshot =
      seal_source(source_origin("recipe.yml"), declaration(true), catalog);

  auto corrupt = encode_source_snapshot(snapshot);
  corrupt[20] ^= 0x01U;
  expect(codec_error_code::checksum_mismatch, [&] {
    (void)decode_source_snapshot(corrupt);
  });

  auto truncated = encode_source_snapshot(snapshot);
  truncated.resize(17);
  expect(codec_error_code::truncated, [&] {
    (void)decode_source_snapshot(truncated);
  });

  auto bad_magic = encode_source_snapshot(snapshot);
  bad_magic[0] = 'X';
  reseal_checksum(bad_magic);
  expect(codec_error_code::invalid_magic, [&] {
    (void)decode_source_snapshot(bad_magic);
  });

  auto bad_version = encode_source_snapshot(snapshot);
  bad_version[8] = 0;
  bad_version[9] = 2;
  reseal_checksum(bad_version);
  expect(codec_error_code::unsupported_version, [&] {
    (void)decode_source_snapshot(bad_version);
  });

  auto trailing = encode_source_snapshot(snapshot);
  trailing.insert(trailing.end() - 32, 0U);
  reseal_checksum(trailing);
  expect(codec_error_code::invalid_record, [&] {
    (void)decode_source_snapshot(trailing);
  });

  std::vector<std::uint8_t> oversized(
      maximum_profile_catalog_encoding_size + 1U, 0U);
  expect(codec_error_code::size_limit, [&] {
    (void)decode_profile_catalog(oversized);
  });
}

void test_identity_refusals()
{
  const auto catalog = profiles();
  const auto snapshot =
      seal_source(source_origin("recipe.yml"), declaration(true), catalog);

  auto wrong_source_identity = encode_source_snapshot(snapshot);
  std::size_t offset = 10;
  skip_text(wrong_source_identity, offset);
  const auto identity_size = u32(wrong_source_identity, offset);
  assert(identity_size == 64);
  offset += 4;
  wrong_source_identity[offset] =
      wrong_source_identity[offset] == 'a' ? 'b' : 'a';
  reseal_checksum(wrong_source_identity);
  expect(codec_error_code::identity_mismatch, [&] {
    (void)decode_source_snapshot(wrong_source_identity);
  });

  auto wrong_profile_identity = encode_profile_catalog(catalog);
  offset = 14;
  skip_text(wrong_profile_identity, offset);
  const auto profile_identity_size = u32(wrong_profile_identity, offset);
  assert(profile_identity_size == 64);
  offset += 4;
  wrong_profile_identity[offset] =
      wrong_profile_identity[offset] == 'a' ? 'b' : 'a';
  reseal_checksum(wrong_profile_identity);
  expect(codec_error_code::identity_mismatch, [&] {
    (void)decode_profile_catalog(wrong_profile_identity);
  });
}

void test_nested_and_value_refusals()
{
  const auto catalog = profiles();
  const auto snapshot =
      seal_source(source_origin("recipe.yml"), declaration(true), catalog);

  auto broken_embedded_profile = encode_source_snapshot(snapshot);
  const auto layout = inspect_source_record(broken_embedded_profile);
  assert(layout.profile_blob_size > 42);
  broken_embedded_profile[layout.profile_blob + 10] ^= 0x01U;
  reseal_checksum(broken_embedded_profile);
  expect(codec_error_code::checksum_mismatch, [&] {
    (void)decode_source_snapshot(broken_embedded_profile);
  });

  auto invalid_presence = encode_source_snapshot(snapshot);
  const auto invalid_presence_layout = inspect_source_record(invalid_presence);
  invalid_presence[invalid_presence_layout.description_flag] = 2U;
  reseal_checksum(invalid_presence);
  expect(codec_error_code::invalid_record, [&] {
    (void)decode_source_snapshot(invalid_presence);
  });

  auto invalid_source_kind = encode_source_snapshot(snapshot);
  const auto invalid_kind_layout = inspect_source_record(invalid_source_kind);
  assert(!invalid_kind_layout.sources.empty());
  invalid_source_kind[invalid_kind_layout.sources.front().first] = 9U;
  reseal_checksum(invalid_source_kind);
  expect(codec_error_code::invalid_record, [&] {
    (void)decode_source_snapshot(invalid_source_kind);
  });

  auto excessive_count = encode_profile_catalog(profile_catalog::seal({}));
  set_u32(excessive_count, 10, maximum_record_item_count + 1U);
  reseal_checksum(excessive_count);
  expect(codec_error_code::size_limit, [&] {
    (void)decode_profile_catalog(excessive_count);
  });
}

void test_noncanonical_source_order()
{
  const auto catalog = profiles();
  const auto snapshot =
      seal_source(source_origin("recipe.yml"), declaration(false), catalog);
  const auto canonical = encode_source_snapshot(snapshot);
  auto reordered = canonical;
  const auto layout = inspect_source_record(reordered);
  assert(u32(reordered, layout.source_count) == 2U);
  assert(layout.sources.size() == 2U);

  const auto first = std::vector<std::uint8_t>(
      reordered.begin() + static_cast<std::ptrdiff_t>(layout.sources[0].first),
      reordered.begin() +
          static_cast<std::ptrdiff_t>(layout.sources[0].second));
  const auto second = std::vector<std::uint8_t>(
      reordered.begin() + static_cast<std::ptrdiff_t>(layout.sources[1].first),
      reordered.begin() +
          static_cast<std::ptrdiff_t>(layout.sources[1].second));
  auto output =
      reordered.begin() + static_cast<std::ptrdiff_t>(layout.sources[0].first);
  output = std::copy(second.begin(), second.end(), output);
  std::copy(first.begin(), first.end(), output);
  reseal_checksum(reordered);
  assert(reordered != canonical);

  expect(codec_error_code::noncanonical, [&] {
    (void)decode_source_snapshot(reordered);
  });
}

} // namespace

int main()
{
  test_golden_vectors();
  test_profile_catalog_round_trip();
  test_source_snapshot_round_trip();
  test_envelope_refusals();
  test_identity_refusals();
  test_nested_and_value_refusals();
  test_noncanonical_source_order();
}
