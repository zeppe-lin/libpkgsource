// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource/libpkgsource.h>

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace pkgsource;

namespace {

declaration_provenance at(const char* document, const char* path,
                          std::uint32_t line)
{
  return declaration_provenance(document, path, line, 3);
}

profile_catalog profiles()
{
  return profile_catalog::seal({
      profile_declaration(
          profile_reference("@compiler"), at("profiles.yml", "compiler", 1),
          {profile_member_declaration(
              requirement_subject(package_reference("gcc")),
              at("profiles.yml", "compiler[0]", 2))}),
      profile_declaration(
          profile_reference("@toolchain"), at("profiles.yml", "toolchain", 4),
          {
            profile_member_declaration(
                requirement_subject(package_reference("binutils")),
                at("profiles.yml", "toolchain[0]", 5)),
            profile_member_declaration(
                requirement_subject(profile_reference("@compiler")),
                at("profiles.yml", "toolchain[1]", 6)),
          }),
      profile_declaration(
          profile_reference("@runtime"), at("profiles.yml", "runtime", 8),
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
      requirement_declaration(
          requirement_scope::run(),
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
  package_metadata metadata(
      "Example package", "Long description", "https://example.invalid",
      {"MIT", "BSD-2-Clause"});
  std::vector<source_input> sources{
      source_input::remote(
          "https://example.invalid/example.tar.xz", "example.tar.xz",
          digest(digest_algorithm::sha256, a)),
      source_input::local(
          "files/example.conf", "example.conf",
          digest(digest_algorithm::sha256, b)),
  };
  program build(program_language::posix_shell, "echo build\n");
  std::vector<lifecycle_program> lifecycle{
      lifecycle_program(
          lifecycle_action::post_install,
          program(program_language::posix_shell,
                  "update-desktop-database\n")),
  };
  architecture_requirements architectures(
      {architecture_reference("x86_64")},
      {architecture_reference("x86_64")});
  auto provenance = at("recipe.yml", "$", 1);

  if (with_check) {
    requirements.emplace_back(
        requirement_scope::check(),
        requirement_subject(package_reference("pkgcheck")),
        at("recipe.yml", "requirements.check[0]", 15));
    return recipe_declaration(
        std::move(release), std::move(metadata), std::move(sources),
        std::move(build), std::move(requirements), std::move(lifecycle),
        std::move(architectures), std::move(provenance),
        program(program_language::posix_shell, "meson test -C build\n"));
  }
  return recipe_declaration(
      std::move(release), std::move(metadata), std::move(sources),
      std::move(build), std::move(requirements), std::move(lifecycle),
      std::move(architectures), std::move(provenance));
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

std::array<std::uint8_t, 32> checksum(
    const std::vector<std::uint8_t>& value, std::size_t size)
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

void reseal_checksum(std::vector<std::uint8_t>& value)
{
  assert(value.size() >= 32);
  const auto digest = checksum(value, value.size() - 32);
  std::copy(digest.begin(), digest.end(), value.end() - 32);
}

std::uint32_t u32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
  return static_cast<std::uint32_t>(bytes[offset]) << 24U |
         static_cast<std::uint32_t>(bytes[offset + 1]) << 16U |
         static_cast<std::uint32_t>(bytes[offset + 2]) << 8U |
         static_cast<std::uint32_t>(bytes[offset + 3]);
}

void test_profile_catalog_round_trip()
{
  const auto catalog = profiles();
  const auto encoding = encode_profile_catalog(catalog);
  assert(encoding.size() > 42);
  assert(std::equal(encoding.begin(), encoding.begin() + 8,
                    std::array<std::uint8_t, 8>{
                        'Z','L','P','S','P','C','A','T'}.begin()));
  const auto decoded = decode_profile_catalog(encoding);
  assert(decoded.profiles().size() == 3);
  assert(decoded.require(profile_reference("@toolchain")).expansion().size()
         == 2);
  assert(decoded.require(profile_reference("@toolchain")).identity()
         == catalog.require(profile_reference("@toolchain")).identity());
  assert(encode_profile_catalog(decoded) == encoding);

  auto reordered = profile_catalog::seal({
      profile_declaration(
          profile_reference("@runtime"), at("profiles.yml", "runtime", 8),
          {profile_member_declaration(
              requirement_subject(package_reference("libfoo")),
              at("profiles.yml", "runtime[0]", 9))}),
      profile_declaration(
          profile_reference("@toolchain"), at("profiles.yml", "toolchain", 4),
          {
            profile_member_declaration(
                requirement_subject(profile_reference("@compiler")),
                at("profiles.yml", "toolchain[1]", 6)),
            profile_member_declaration(
                requirement_subject(package_reference("binutils")),
                at("profiles.yml", "toolchain[0]", 5)),
          }),
      profile_declaration(
          profile_reference("@compiler"), at("profiles.yml", "compiler", 1),
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
      source_origin("recipes/example/recipe.yml"),
      declaration(false), catalog);
  const auto plain_encoding = encode_source_snapshot(plain);
  const auto plain_decoded = decode_source_snapshot(plain_encoding);
  assert(plain_decoded.identity() == plain.identity());
  assert(plain_decoded.origin().document() == plain.origin().document());
  assert(plain_decoded.recipe().build_requirements().size() == 2);
  assert(plain_decoded.recipe().run_requirements().size() == 1);
  assert(plain_decoded.recipe().profile_closure().size() == 3);
  assert(encode_source_snapshot(plain_decoded) == plain_encoding);

  const auto checked = seal_source(
      source_origin("recipes/example/recipe.yml"),
      declaration(true), catalog);
  const auto checked_encoding = encode_source_snapshot(checked);
  const auto checked_decoded = decode_source_snapshot(checked_encoding);
  assert(checked_decoded.identity() == checked.identity());
  assert(checked_decoded.recipe().check_program());
  assert(checked_decoded.recipe().check_program()->material()
         == "meson test -C build\n");
  assert(encode_source_snapshot(checked_decoded) == checked_encoding);
}

void test_refusals()
{
  const auto catalog = profiles();
  const auto snapshot = seal_source(
      source_origin("recipe.yml"),
      declaration(true), catalog);

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

  auto wrong_identity = encode_source_snapshot(snapshot);
  std::size_t offset = 10;
  const auto origin_size = u32(wrong_identity, offset);
  offset += 4 + origin_size;
  const auto identity_size = u32(wrong_identity, offset);
  assert(identity_size == 64);
  offset += 4;
  wrong_identity[offset] = wrong_identity[offset] == 'a' ? 'b' : 'a';
  reseal_checksum(wrong_identity);
  expect(codec_error_code::identity_mismatch, [&] {
    (void)decode_source_snapshot(wrong_identity);
  });

  auto profile = encode_profile_catalog(catalog);
  profile[profile.size() - 1] ^= 0x01U;
  expect(codec_error_code::checksum_mismatch, [&] {
    (void)decode_profile_catalog(profile);
  });
}

} // namespace

int main()
{
  test_profile_catalog_round_trip();
  test_source_snapshot_round_trip();
  test_refusals();
}
