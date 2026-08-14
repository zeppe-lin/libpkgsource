// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource/error.h>
#include <libpkgsource/model.h>

#include "internal/identity_hex.h"
#include "internal/identity_writer.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <tuple>
#include <utility>

namespace pkgsource {
namespace {

bool canonical_atom(std::string_view value, bool allow_at)
{
  std::size_t offset = 0;
  if (allow_at) {
    if (value.empty() || value.front() != '@') {
      return false;
    }
    offset = 1;
  }
  if (offset == value.size()) {
    return false;
  }
  const auto valid_first = [](char c) {
    return c >= 'a' && c <= 'z';
  };
  const auto valid_rest = [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' ||
           c == '.' || c == '_' || c == '-';
  };
  if (!valid_first(value[offset])) {
    return false;
  }
  for (std::size_t i = offset + 1; i < value.size(); ++i) {
    if (!valid_rest(value[i])) {
      return false;
    }
  }
  return true;
}

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

bool text_safe(std::string_view value)
{
  if (value.empty()) {
    return false;
  }
  for (unsigned char c : value) {
    if (c == 0 || (c < 0x20 && c != '\n' && c != '\t') || c == 0x7f) {
      return false;
    }
  }
  return true;
}

bool valid_digest_algorithm(digest_algorithm value) noexcept
{
  return value == digest_algorithm::sha256;
}

bool valid_requirement_scope_kind(requirement_scope_kind value) noexcept
{
  switch (value) {
  case requirement_scope_kind::build:
  case requirement_scope_kind::run:
  case requirement_scope_kind::check:
  case requirement_scope_kind::lifecycle:
    return true;
  }
  return false;
}

bool valid_lifecycle_action(lifecycle_action value) noexcept
{
  switch (value) {
  case lifecycle_action::pre_install:
  case lifecycle_action::post_install:
  case lifecycle_action::pre_remove:
  case lifecycle_action::post_remove:
    return true;
  }
  return false;
}

bool valid_program_language(program_language value) noexcept
{
  return value == program_language::posix_shell;
}

bool safe_basename(std::string_view value)
{
  return line_safe(value) && value != "." && value != ".." &&
         value.find('/') == std::string_view::npos;
}

bool safe_relative_path(std::string_view value)
{
  if (!line_safe(value) || value.front() == '/') {
    return false;
  }
  std::size_t start = 0;
  while (start <= value.size()) {
    const std::size_t end = value.find('/', start);
    const std::string_view part = value.substr(
        start,
        end == std::string_view::npos ? value.size() - start : end - start);
    if (part.empty() || part == "." || part == "..") {
      return false;
    }
    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1;
  }
  return true;
}

template <typename T>
void sort_unique(std::vector<T>& values, std::string_view field)
{
  std::sort(values.begin(), values.end());
  if (std::adjacent_find(values.begin(), values.end()) != values.end()) {
    throw error(error_code::duplicate_declaration,
                "duplicate normalized " + std::string(field));
  }
}

package_release_identity make_release_identity(const package_reference& package,
                                               std::string_view version,
                                               std::uint32_t release)
{
  detail::identity_writer writer;
  writer.text("libpkgsource/package-release/v1");
  writer.text(package.name());
  writer.text(version);
  writer.number(release);
  return package_release_identity::from_sha256(writer.finish());
}

package_release_identity
validated_release_identity(const package_reference& package,
                           std::string_view version,
                           std::uint32_t release)
{
  if (!line_safe(version) || version.find('/') != std::string_view::npos ||
      release == 0) {
    throw error(error_code::invalid_metadata,
                "invalid package version or release");
  }
  return make_release_identity(package, version, release);
}

digest validated_program_digest(program_language language,
                                std::string_view material)
{
  if (!valid_program_language(language) || !text_safe(material)) {
    throw error(error_code::invalid_program,
                "invalid program language or empty/binary material");
  }
  return digest(digest_algorithm::sha256, detail::sha256_hex(material));
}

} // namespace

std::string_view to_string(digest_algorithm value) noexcept
{
  switch (value) {
  case digest_algorithm::sha256:
    return "sha256";
  }
  return "unknown";
}
std::string_view to_string(requirement_scope_kind value) noexcept
{
  switch (value) {
  case requirement_scope_kind::build:
    return "build";
  case requirement_scope_kind::run:
    return "run";
  case requirement_scope_kind::check:
    return "check";
  case requirement_scope_kind::lifecycle:
    return "lifecycle";
  }
  return "unknown";
}
std::string_view to_string(lifecycle_action value) noexcept
{
  switch (value) {
  case lifecycle_action::pre_install:
    return "pre-install";
  case lifecycle_action::post_install:
    return "post-install";
  case lifecycle_action::pre_remove:
    return "pre-remove";
  case lifecycle_action::post_remove:
    return "post-remove";
  }
  return "unknown";
}
std::string_view to_string(requirement_subject_kind value) noexcept
{
  switch (value) {
  case requirement_subject_kind::package:
    return "package";
  case requirement_subject_kind::profile:
    return "profile";
  }
  return "unknown";
}
std::string_view to_string(source_input_kind value) noexcept
{
  switch (value) {
  case source_input_kind::remote:
    return "remote";
  case source_input_kind::local:
    return "local";
  }
  return "unknown";
}
std::string_view to_string(source_unpack_kind value) noexcept
{
  switch (value) {
  case source_unpack_kind::none:
    return "none";
  case source_unpack_kind::archive:
    return "archive";
  }
  return "unknown";
}
std::string_view to_string(program_language value) noexcept
{
  switch (value) {
  case program_language::posix_shell:
    return "posix-shell";
  }
  return "unknown";
}

digest::digest(digest_algorithm algorithm, std::string hex)
    : algorithm_(algorithm), hex_(std::move(hex))
{
  if (!valid_digest_algorithm(algorithm_)) {
    throw error(error_code::invalid_identity, "invalid digest algorithm");
  }
  detail::require_sha256_hex(hex_);
}
digest_algorithm digest::algorithm() const noexcept
{
  return algorithm_;
}
const std::string& digest::hex() const noexcept
{
  return hex_;
}
bool operator==(const digest& lhs, const digest& rhs) noexcept
{
  return lhs.algorithm_ == rhs.algorithm_ && lhs.hex_ == rhs.hex_;
}
bool operator!=(const digest& lhs, const digest& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const digest& lhs, const digest& rhs) noexcept
{
  return std::tie(lhs.algorithm_, lhs.hex_) <
         std::tie(rhs.algorithm_, rhs.hex_);
}

package_reference::package_reference(std::string name) : name_(std::move(name))
{
  if (!canonical_atom(name_, false)) {
    throw error(error_code::invalid_identity,
                "invalid canonical package reference: " + name_);
  }
}
const std::string& package_reference::name() const noexcept
{
  return name_;
}
bool operator==(const package_reference& lhs,
                const package_reference& rhs) noexcept
{
  return lhs.name_ == rhs.name_;
}
bool operator!=(const package_reference& lhs,
                const package_reference& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const package_reference& lhs,
               const package_reference& rhs) noexcept
{
  return lhs.name_ < rhs.name_;
}

profile_reference::profile_reference(std::string name) : name_(std::move(name))
{
  if (!canonical_atom(name_, true)) {
    throw error(error_code::invalid_identity,
                "invalid canonical profile reference: " + name_);
  }
}
const std::string& profile_reference::name() const noexcept
{
  return name_;
}
bool operator==(const profile_reference& lhs,
                const profile_reference& rhs) noexcept
{
  return lhs.name_ == rhs.name_;
}
bool operator!=(const profile_reference& lhs,
                const profile_reference& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const profile_reference& lhs,
               const profile_reference& rhs) noexcept
{
  return lhs.name_ < rhs.name_;
}

architecture_reference::architecture_reference(std::string name)
    : name_(std::move(name))
{
  if (!canonical_atom(name_, false)) {
    throw error(error_code::invalid_identity,
                "invalid canonical architecture reference: " + name_);
  }
}
const std::string& architecture_reference::name() const noexcept
{
  return name_;
}
bool operator==(const architecture_reference& lhs,
                const architecture_reference& rhs) noexcept
{
  return lhs.name_ == rhs.name_;
}
bool operator!=(const architecture_reference& lhs,
                const architecture_reference& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const architecture_reference& lhs,
               const architecture_reference& rhs) noexcept
{
  return lhs.name_ < rhs.name_;
}

declaration_provenance::declaration_provenance(std::string document,
                                               std::string path,
                                               std::uint32_t line,
                                               std::uint32_t column)
    : document_(std::move(document)), path_(std::move(path)), line_(line),
      column_(column)
{
  if (!line_safe(document_) || !line_safe(path_) || line_ == 0 ||
      column_ == 0) {
    throw error(error_code::invalid_provenance,
                "invalid declaration provenance");
  }
}
const std::string& declaration_provenance::document() const noexcept
{
  return document_;
}
const std::string& declaration_provenance::path() const noexcept
{
  return path_;
}
std::uint32_t declaration_provenance::line() const noexcept
{
  return line_;
}
std::uint32_t declaration_provenance::column() const noexcept
{
  return column_;
}
bool operator==(const declaration_provenance& lhs,
                const declaration_provenance& rhs) noexcept
{
  return std::tie(lhs.document_, lhs.path_, lhs.line_, lhs.column_) ==
         std::tie(rhs.document_, rhs.path_, rhs.line_, rhs.column_);
}
bool operator!=(const declaration_provenance& lhs,
                const declaration_provenance& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const declaration_provenance& lhs,
               const declaration_provenance& rhs) noexcept
{
  return std::tie(lhs.document_, lhs.path_, lhs.line_, lhs.column_) <
         std::tie(rhs.document_, rhs.path_, rhs.line_, rhs.column_);
}

requirement_scope::requirement_scope(requirement_scope_kind kind,
                                     std::optional<lifecycle_action> action)
    : kind_(kind), action_(action)
{
  if (!valid_requirement_scope_kind(kind_) ||
      (kind_ == requirement_scope_kind::lifecycle) != action_.has_value() ||
      (action_ && !valid_lifecycle_action(*action_))) {
    throw error(error_code::invalid_requirement,
                "invalid lifecycle requirement scope binding");
  }
}
requirement_scope requirement_scope::build()
{
  return requirement_scope(requirement_scope_kind::build, std::nullopt);
}
requirement_scope requirement_scope::run()
{
  return requirement_scope(requirement_scope_kind::run, std::nullopt);
}
requirement_scope requirement_scope::check()
{
  return requirement_scope(requirement_scope_kind::check, std::nullopt);
}
requirement_scope requirement_scope::lifecycle(lifecycle_action action)
{
  return requirement_scope(requirement_scope_kind::lifecycle, action);
}
requirement_scope_kind requirement_scope::kind() const noexcept
{
  return kind_;
}
const std::optional<lifecycle_action>&
requirement_scope::action() const noexcept
{
  return action_;
}
bool operator==(const requirement_scope& lhs,
                const requirement_scope& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.action_) == std::tie(rhs.kind_, rhs.action_);
}
bool operator!=(const requirement_scope& lhs,
                const requirement_scope& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const requirement_scope& lhs,
               const requirement_scope& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.action_) < std::tie(rhs.kind_, rhs.action_);
}

requirement_subject::requirement_subject(package_reference package)
    : kind_(requirement_subject_kind::package), package_(std::move(package))
{
}
requirement_subject::requirement_subject(profile_reference profile)
    : kind_(requirement_subject_kind::profile), profile_(std::move(profile))
{
}
requirement_subject_kind requirement_subject::kind() const noexcept
{
  return kind_;
}
const package_reference& requirement_subject::package() const
{
  if (!package_) {
    throw error(error_code::invalid_request,
                "requirement subject is not a package reference");
  }
  return *package_;
}
const profile_reference& requirement_subject::profile() const
{
  if (!profile_) {
    throw error(error_code::invalid_request,
                "requirement subject is not a profile reference");
  }
  return *profile_;
}
std::string requirement_subject::text() const
{
  return kind_ == requirement_subject_kind::package ? package_->name()
                                                    : profile_->name();
}
bool operator==(const requirement_subject& lhs,
                const requirement_subject& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.package_, lhs.profile_) ==
         std::tie(rhs.kind_, rhs.package_, rhs.profile_);
}
bool operator!=(const requirement_subject& lhs,
                const requirement_subject& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const requirement_subject& lhs,
               const requirement_subject& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.package_, lhs.profile_) <
         std::tie(rhs.kind_, rhs.package_, rhs.profile_);
}

requirement_declaration::requirement_declaration(
    requirement_scope scope,
    requirement_subject subject,
    declaration_provenance provenance)
    : scope_(std::move(scope)), subject_(std::move(subject)),
      provenance_(std::move(provenance))
{
}
const requirement_scope& requirement_declaration::scope() const noexcept
{
  return scope_;
}
const requirement_subject& requirement_declaration::subject() const noexcept
{
  return subject_;
}
const declaration_provenance&
requirement_declaration::provenance() const noexcept
{
  return provenance_;
}

package_release::package_release(package_reference package,
                                 std::string version,
                                 std::uint32_t release)
    : package_(std::move(package)), version_(std::move(version)),
      release_(release),
      identity_(validated_release_identity(package_, version_, release_))
{
}
const package_reference& package_release::package() const noexcept
{
  return package_;
}
const std::string& package_release::version() const noexcept
{
  return version_;
}
std::uint32_t package_release::release() const noexcept
{
  return release_;
}
const package_release_identity& package_release::identity() const noexcept
{
  return identity_;
}
std::string package_release::version_release() const
{
  return version_ + "-" + std::to_string(release_);
}

package_metadata::package_metadata(std::string summary,
                                   std::optional<std::string> description,
                                   std::optional<std::string> homepage,
                                   std::vector<std::string> licenses)
    : summary_(std::move(summary)), description_(std::move(description)),
      homepage_(std::move(homepage)), licenses_(std::move(licenses))
{
  if (!line_safe(summary_) || (description_ && !text_safe(*description_)) ||
      (homepage_ && !line_safe(*homepage_)) || licenses_.empty()) {
    throw error(error_code::invalid_metadata, "invalid package metadata");
  }
  for (const std::string& license : licenses_) {
    if (!line_safe(license)) {
      throw error(error_code::invalid_metadata, "invalid package license");
    }
  }
  sort_unique(licenses_, "package license");
}
const std::string& package_metadata::summary() const noexcept
{
  return summary_;
}
const std::optional<std::string>& package_metadata::description() const noexcept
{
  return description_;
}
const std::optional<std::string>& package_metadata::homepage() const noexcept
{
  return homepage_;
}
const std::vector<std::string>& package_metadata::licenses() const noexcept
{
  return licenses_;
}

source_input::source_input(source_input_kind kind,
                           std::string location,
                           std::string local_name,
                           digest content_digest)
    : source_input(kind, std::move(location), std::move(local_name),
                   std::move(content_digest), source_unpack_kind::none)
{
}
source_input::source_input(source_input_kind kind,
                           std::string location,
                           std::string local_name,
                           digest content_digest,
                           source_unpack_kind unpack)
    : kind_(kind), unpack_(unpack), location_(std::move(location)),
      local_name_(std::move(local_name)),
      content_digest_(std::move(content_digest))
{
  if (kind_ != source_input_kind::remote &&
      kind_ != source_input_kind::local) {
    throw error(error_code::invalid_source, "invalid source input kind");
  }
  if (unpack_ != source_unpack_kind::none &&
      unpack_ != source_unpack_kind::archive) {
    throw error(error_code::invalid_source, "invalid source unpack policy");
  }
  if (!safe_basename(local_name_)) {
    throw error(error_code::invalid_source, "invalid source local name");
  }
  if (kind_ == source_input_kind::remote) {
    if (!line_safe(location_) || location_.find("://") == std::string::npos) {
      throw error(error_code::invalid_source, "invalid remote source URL");
    }
  } else if (!safe_relative_path(location_)) {
    throw error(error_code::invalid_source, "invalid local source path");
  }
}
source_input source_input::remote(std::string url,
                                  std::string local_name,
                                  digest content_digest)
{
  return remote(std::move(url), std::move(local_name),
                std::move(content_digest), source_unpack_kind::none);
}
source_input source_input::remote(std::string url,
                                  std::string local_name,
                                  digest content_digest,
                                  source_unpack_kind unpack)
{
  return source_input(source_input_kind::remote, std::move(url),
                      std::move(local_name), std::move(content_digest), unpack);
}
source_input source_input::local(std::string path,
                                 std::string local_name,
                                 digest content_digest)
{
  return local(std::move(path), std::move(local_name),
               std::move(content_digest), source_unpack_kind::none);
}
source_input source_input::local(std::string path,
                                 std::string local_name,
                                 digest content_digest,
                                 source_unpack_kind unpack)
{
  return source_input(source_input_kind::local, std::move(path),
                      std::move(local_name), std::move(content_digest), unpack);
}
source_input_kind source_input::kind() const noexcept
{
  return kind_;
}
const std::string& source_input::location() const noexcept
{
  return location_;
}
const std::string& source_input::local_name() const noexcept
{
  return local_name_;
}
const digest& source_input::content_digest() const noexcept
{
  return content_digest_;
}
source_unpack_kind source_input::unpack_kind() const noexcept
{
  return unpack_;
}
bool operator==(const source_input& lhs, const source_input& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.unpack_, lhs.location_, lhs.local_name_,
                  lhs.content_digest_) ==
         std::tie(rhs.kind_, rhs.unpack_, rhs.location_, rhs.local_name_,
                  rhs.content_digest_);
}
bool operator!=(const source_input& lhs, const source_input& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const source_input& lhs, const source_input& rhs) noexcept
{
  return std::tie(lhs.local_name_, lhs.kind_, lhs.unpack_, lhs.location_,
                  lhs.content_digest_) <
         std::tie(rhs.local_name_, rhs.kind_, rhs.unpack_, rhs.location_,
                  rhs.content_digest_);
}

program::program(program_language language, std::string material)
    : language_(language), material_(std::move(material)),
      content_digest_(validated_program_digest(language_, material_))
{
}
program_language program::language() const noexcept
{
  return language_;
}
const std::string& program::material() const noexcept
{
  return material_;
}
const digest& program::content_digest() const noexcept
{
  return content_digest_;
}
bool operator==(const program& lhs, const program& rhs) noexcept
{
  return std::tie(lhs.language_, lhs.material_, lhs.content_digest_) ==
         std::tie(rhs.language_, rhs.material_, rhs.content_digest_);
}
bool operator!=(const program& lhs, const program& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const program& lhs, const program& rhs) noexcept
{
  return std::tie(lhs.language_, lhs.material_, lhs.content_digest_) <
         std::tie(rhs.language_, rhs.material_, rhs.content_digest_);
}

lifecycle_program::lifecycle_program(lifecycle_action action, program value)
    : action_(action), value_(std::move(value))
{
  if (!valid_lifecycle_action(action_)) {
    throw error(error_code::invalid_program, "invalid lifecycle action");
  }
}
lifecycle_action lifecycle_program::action() const noexcept
{
  return action_;
}
const program& lifecycle_program::value() const noexcept
{
  return value_;
}
bool operator==(const lifecycle_program& lhs,
                const lifecycle_program& rhs) noexcept
{
  return std::tie(lhs.action_, lhs.value_) == std::tie(rhs.action_, rhs.value_);
}
bool operator!=(const lifecycle_program& lhs,
                const lifecycle_program& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const lifecycle_program& lhs,
               const lifecycle_program& rhs) noexcept
{
  return std::tie(lhs.action_, lhs.value_) < std::tie(rhs.action_, rhs.value_);
}

architecture_requirements::architecture_requirements(
    std::vector<architecture_reference> build,
    std::vector<architecture_reference> target)
    : build_(std::move(build)), target_(std::move(target))
{
  sort_unique(build_, "build architecture");
  sort_unique(target_, "target architecture");
}
const std::vector<architecture_reference>&
architecture_requirements::build() const noexcept
{
  return build_;
}
const std::vector<architecture_reference>&
architecture_requirements::target() const noexcept
{
  return target_;
}

} // namespace pkgsource
