// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

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

namespace test_support::codec_fixture {

using namespace pkgsource;
using namespace pkgsource::codec;

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
  ++offset; // unpack policy
  assert(offset < bytes.size());
  ++offset; // digest algorithm
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

} // namespace test_support::codec_fixture
