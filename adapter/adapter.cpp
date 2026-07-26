// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource-plan/adapter.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

#include <openssl/evp.h>

#include <libpkgplan/control.h>

namespace pkgsource::plan_adapter {
namespace {

class canonical_record final {
public:
  void u8(std::uint8_t value) { bytes_.push_back(value); }

  void u64(std::uint64_t value)
  {
    for (int shift = 56; shift >= 0; shift -= 8)
      bytes_.push_back(static_cast<std::uint8_t>(value >> shift));
  }

  void text(std::string_view value)
  {
    u64(static_cast<std::uint64_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void material(const std::string& value)
  {
    u64(static_cast<std::uint64_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept
  {
    return bytes_;
  }

private:
  std::vector<std::uint8_t> bytes_;
};

[[nodiscard]] pkgplan::sha256_digest_bytes
sha256(const canonical_record& record)
{
  pkgplan::sha256_digest_bytes digest{};
  unsigned int size = 0;
  EVP_MD_CTX* context = EVP_MD_CTX_new();
  if (context == nullptr)
    throw projection_error(projection_error_code::planner_fact,
                           "cannot allocate source planning digest context");

  const auto free_context = [&]() noexcept { EVP_MD_CTX_free(context); };
  if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(context, record.bytes().data(),
                       record.bytes().size()) != 1 ||
      EVP_DigestFinal_ex(context, digest.data(), &size) != 1)
  {
    free_context();
    throw projection_error(projection_error_code::planner_fact,
                           "cannot compute source planning identity");
  }
  free_context();

  if (size != digest.size())
    throw projection_error(projection_error_code::planner_fact,
                           "source planning identity has unexpected width");
  return digest;
}

[[nodiscard]] std::string
read_exact(const pkgsource::captured_file& file)
{
  if (file.size() > static_cast<std::uintmax_t>(
                        std::numeric_limits<std::size_t>::max()) ||
      file.size() > static_cast<std::uintmax_t>(
                        std::numeric_limits<std::streamsize>::max()))
    throw projection_error(projection_error_code::lifecycle_read,
                           "captured lifecycle program is too large");

  std::ifstream input(file.native_path(), std::ios::binary);
  if (!input)
    throw projection_error(projection_error_code::lifecycle_read,
                           "cannot open captured lifecycle program");

  std::string material(static_cast<std::size_t>(file.size()), '\0');
  if (!material.empty())
    input.read(material.data(), static_cast<std::streamsize>(material.size()));
  if (!input || input.peek() != std::ifstream::traits_type::eof())
    throw projection_error(projection_error_code::lifecycle_read,
                           "cannot read exact captured lifecycle program");
  return material;
}

[[nodiscard]] pkgplan::package_release_identity
release_identity(const pkgsource::package_identity& source)
{
  canonical_record record;
  record.text("libpkgsource-plan/package-release/v1");
  record.text(source.name());
  record.text(source.version());
  record.text(source.release());
  return pkgplan::package_release_identity::from_sha256(sha256(record));
}

[[nodiscard]] pkgplan::candidate_control_identity
control_identity(const pkgplan::candidate_control_projection& control)
{
  canonical_record record;
  record.text("libpkgsource-plan/candidate-control/v1");

  record.u64(control.runtime_dependencies().size());
  for (const auto& dependency : control.runtime_dependencies())
    record.text(dependency.expression());

  record.u64(control.removal_lifecycle().size());
  for (const auto& lifecycle : control.removal_lifecycle())
  {
    record.u8(static_cast<std::uint8_t>(lifecycle.phase()));
    record.text(lifecycle.format());
    record.material(lifecycle.material());
  }

  record.u64(control.target_profile().size());
  for (const auto& fact : control.target_profile())
  {
    record.text(fact.name());
    record.text(fact.value());
  }

  return pkgplan::candidate_control_identity::from_sha256(sha256(record));
}

[[nodiscard]] pkgplan::removal_lifecycle_phase
translate_phase(pkgsource::lifecycle_phase phase)
{
  switch (phase)
  {
    case pkgsource::lifecycle_phase::pre_remove:
      return pkgplan::removal_lifecycle_phase::pre_remove;
    case pkgsource::lifecycle_phase::post_remove:
      return pkgplan::removal_lifecycle_phase::post_remove;
    case pkgsource::lifecycle_phase::pre_install:
    case pkgsource::lifecycle_phase::post_install:
      break;
  }
  throw projection_error(projection_error_code::planner_fact,
                         "installation lifecycle entered removal projection");
}

} // namespace

projection_error::projection_error(projection_error_code code,
                                   std::string message)
    : std::runtime_error(std::move(message)), code_(code)
{
}

projection_error_code
projection_error::code() const noexcept
{
  return code_;
}

candidate_projection::candidate_projection(
    pkgsource::source_snapshot source,
    pkgplan::candidate_package_fact candidate)
    : source_(std::move(source)), candidate_(std::move(candidate))
{
}

const pkgsource::source_snapshot&
candidate_projection::source() const noexcept
{
  return source_;
}

const pkgsource::digest&
candidate_projection::source_fingerprint() const noexcept
{
  return source_.fingerprint();
}

const pkgplan::candidate_package_fact&
candidate_projection::candidate() const noexcept
{
  return candidate_;
}

candidate_projection
project_candidate(pkgsource::source_snapshot source)
{
  try
  {
    const pkgsource::build_description& build = source.build();

    std::vector<pkgplan::runtime_dependency_declaration> dependencies;
    dependencies.reserve(build.dependencies().size());
    for (const pkgsource::dependency& dependency : build.dependencies())
    {
      if (dependency.scope() != pkgsource::dependency_scope::build_and_run)
        throw projection_error(projection_error_code::planner_fact,
                               "unsupported source dependency scope");
      dependencies.push_back(
          pkgplan::runtime_dependency_declaration::make(dependency.name()));
    }

    std::vector<pkgplan::removal_lifecycle_declaration> lifecycle;
    for (const pkgsource::lifecycle_action& action : build.lifecycle_actions())
    {
      if (action.phase() == pkgsource::lifecycle_phase::pre_install ||
          action.phase() == pkgsource::lifecycle_phase::post_install)
        continue;
      lifecycle.push_back(pkgplan::removal_lifecycle_declaration::make(
          translate_phase(action.phase()), "text/x-shellscript",
          read_exact(action.program())));
    }

    std::vector<pkgplan::target_profile_fact> target_profile;
    target_profile.push_back(pkgplan::target_profile_fact::make(
        "pkgsource.build-architecture",
        pkgsource::to_string(build.architecture())));

    pkgplan::candidate_control_projection control(
        std::move(dependencies), std::move(lifecycle),
        std::move(target_profile));

    const pkgsource::package_identity& identity = build.identity();
    pkgplan::package_release release(
        release_identity(identity), identity.name(), identity.version(),
        identity.release());
    const pkgplan::candidate_control_identity candidate_identity =
        control_identity(control);
    pkgplan::candidate_package_fact candidate(
        candidate_identity, std::move(release), std::move(control));
    return candidate_projection(std::move(source), std::move(candidate));
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::planner_fact,
        std::string("planner rejected source candidate projection: ") +
            error.what());
  }
}

} // namespace pkgsource::plan_adapter
