// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource-plan/adapter.h>

#include <libpkgplan/control.h>
#include <libpkgplan/digest.h>

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace pkgsource::plan_adapter {
namespace {

using context_ptr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

class identity_writer final {
public:
  identity_writer() : context_(EVP_MD_CTX_new(), EVP_MD_CTX_free)
  {
    if (!context_
        || EVP_DigestInit_ex(context_.get(), EVP_sha256(), nullptr) != 1)
      throw projection_error(projection_error_code::identity,
                             "SHA-256 initialization failed");
  }

  void number(std::uint64_t value)
  {
    std::array<unsigned char, 8> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i)
      bytes[bytes.size() - 1 - i] =
          static_cast<unsigned char>(value >> (i * 8));
    update(bytes.data(), bytes.size());
  }

  void text(std::string_view value)
  {
    number(value.size());
    update(value.data(), value.size());
  }

  pkgplan::sha256_digest_bytes finish()
  {
    pkgplan::sha256_digest_bytes bytes{};
    unsigned int size = 0;
    if (EVP_DigestFinal_ex(context_.get(), bytes.data(), &size) != 1
        || size != bytes.size())
      throw projection_error(projection_error_code::identity,
                             "SHA-256 finalization failed");
    return bytes;
  }

private:
  void update(const void* data, std::size_t size)
  {
    if (EVP_DigestUpdate(context_.get(), data, size) != 1)
      throw projection_error(projection_error_code::identity,
                             "SHA-256 update failed");
  }

  context_ptr context_;
};

pkgplan::sha256_digest_bytes parse_sha256(std::string_view hex)
{
  if (hex.size() != 64)
    throw projection_error(projection_error_code::identity,
                           "source identity has invalid width");
  pkgplan::sha256_digest_bytes bytes{};
  const auto nibble = [](char c) -> unsigned int {
    if (c >= '0' && c <= '9')
      return static_cast<unsigned int>(c - '0');
    if (c >= 'a' && c <= 'f')
      return static_cast<unsigned int>(c - 'a' + 10);
    throw projection_error(projection_error_code::identity,
                           "source identity is not lowercase hexadecimal");
  };
  for (std::size_t i = 0; i < bytes.size(); ++i)
    bytes[i] = static_cast<std::uint8_t>(
        (nibble(hex[i * 2]) << 4) | nibble(hex[i * 2 + 1]));
  return bytes;
}

std::string architecture_value(
    const std::vector<pkgsource::architecture_reference>& architectures)
{
  if (architectures.empty())
    return "*";
  std::string result;
  for (const pkgsource::architecture_reference& architecture : architectures) {
    if (!result.empty())
      result += ',';
    result += architecture.name();
  }
  return result;
}

pkgplan::candidate_control_identity control_identity(
    const pkgplan::candidate_control_projection& control)
{
  identity_writer writer;
  writer.text("libpkgsource-plan/candidate-control/v2");
  writer.number(control.runtime_dependencies().size());
  for (const auto& dependency : control.runtime_dependencies())
    writer.text(dependency.expression());
  writer.number(control.removal_lifecycle().size());
  for (const auto& lifecycle : control.removal_lifecycle()) {
    writer.number(static_cast<std::uint64_t>(lifecycle.phase()));
    writer.text(lifecycle.format());
    writer.text(lifecycle.material());
  }
  writer.number(control.target_profile().size());
  for (const auto& profile : control.target_profile()) {
    writer.text(profile.name());
    writer.text(profile.value());
  }
  return pkgplan::candidate_control_identity::from_sha256(writer.finish());
}

pkgplan::removal_lifecycle_phase translate_phase(lifecycle_action action)
{
  switch (action) {
    case lifecycle_action::pre_remove:
      return pkgplan::removal_lifecycle_phase::pre_remove;
    case lifecycle_action::post_remove:
      return pkgplan::removal_lifecycle_phase::post_remove;
    case lifecycle_action::pre_install:
    case lifecycle_action::post_install:
      break;
  }
  throw projection_error(projection_error_code::planner_fact,
                         "installation lifecycle is not durable removal control");
}

} // namespace

projection_error::projection_error(projection_error_code code,
                                   std::string message)
    : std::runtime_error(std::move(message)), code_(code)
{
}
projection_error_code projection_error::code() const noexcept { return code_; }

candidate_projection::candidate_projection(
    pkgsource::source_snapshot source,
    pkgplan::candidate_package_fact candidate)
    : source_(std::move(source)), candidate_(std::move(candidate))
{
}
const pkgsource::source_snapshot& candidate_projection::source() const noexcept
{
  return source_;
}
const pkgsource::source_snapshot_identity&
candidate_projection::source_identity() const noexcept
{
  return source_.identity();
}
const pkgplan::candidate_package_fact&
candidate_projection::candidate() const noexcept
{
  return candidate_;
}

candidate_projection project_candidate(pkgsource::source_snapshot source)
{
  try {
    const sealed_recipe& recipe = source.recipe();

    std::vector<pkgplan::runtime_dependency_declaration> dependencies;
    for (const resolved_requirement& requirement : recipe.run_requirements())
      dependencies.push_back(
          pkgplan::runtime_dependency_declaration::make(
              requirement.package().name()));

    std::vector<pkgplan::removal_lifecycle_declaration> lifecycle;
    for (const lifecycle_action action : {
             lifecycle_action::pre_remove,
             lifecycle_action::post_remove}) {
      const lifecycle_program* value = recipe.lifecycle(action);
      if (!value)
        continue;
      lifecycle.push_back(pkgplan::removal_lifecycle_declaration::make(
          translate_phase(action), "text/x-posix-shell",
          value->value().material()));
    }

    std::vector<pkgplan::target_profile_fact> target_profile;
    target_profile.push_back(pkgplan::target_profile_fact::make(
        "pkgsource.target-architectures",
        architecture_value(recipe.architectures().target())));

    pkgplan::candidate_control_projection control(
        std::move(dependencies), std::move(lifecycle),
        std::move(target_profile));

    const package_release& source_release = recipe.release();
    pkgplan::package_release release(
        pkgplan::package_release_identity::from_sha256(
            parse_sha256(source_release.identity().hex())),
        source_release.package().name(), source_release.version(),
        std::to_string(source_release.release()));
    pkgplan::candidate_package_fact candidate(
        control_identity(control), std::move(release), std::move(control));
    return candidate_projection(std::move(source), std::move(candidate));
  } catch (const projection_error&) {
    throw;
  } catch (const std::exception& value) {
    throw projection_error(
        projection_error_code::planner_fact,
        std::string("planner rejected source candidate projection: ")
            + value.what());
  }
}

} // namespace pkgsource::plan_adapter
