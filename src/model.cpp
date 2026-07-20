// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/model.h>
#include <libpkgsource/error.h>
#include "internal.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <utility>

namespace pkgsource {
namespace {
bool valid_hex(const std::string& value, std::size_t expected) {
  return value.size() == expected && std::all_of(value.begin(), value.end(),
    [](unsigned char c){ return std::isxdigit(c) != 0; });
}
bool valid_atom(const std::string& value) {
  if (value.empty() || value == "." || value == "..") return false;
  return std::none_of(value.begin(), value.end(), [](unsigned char c) {
    return std::iscntrl(c) != 0 || std::isspace(c) != 0 || c == '/';
  });
}

bool valid_text(const std::string& value) {
  return !value.empty() && std::none_of(value.begin(), value.end(),
    [](unsigned char c) { return c == '\0' || (std::iscntrl(c) && c != '\t'); });
}

bool safe_basename(const std::string& value) {
  const std::filesystem::path path(value);
  return valid_text(value) && !path.is_absolute() && path.filename() == path &&
         value != "." && value != "..";
}
}

std::string_view to_string(source_format v) noexcept { return v == source_format::pkgfile_v0 ? "pkgfile/0" : "unknown"; }
std::string_view to_string(digest_algorithm v) noexcept { return v == digest_algorithm::md5 ? "md5" : "sha256"; }
std::string_view to_string(dependency_scope) noexcept { return "build_and_run"; }
std::string_view to_string(source_input_kind v) noexcept { return v == source_input_kind::remote ? "remote" : "recipe_local"; }
std::string_view to_string(recipe_entry_point) noexcept { return "build"; }
std::string_view to_string(lifecycle_phase v) noexcept {
  switch (v) { case lifecycle_phase::pre_install: return "pre_install"; case lifecycle_phase::post_install: return "post_install"; case lifecycle_phase::pre_remove: return "pre_remove"; case lifecycle_phase::post_remove: return "post_remove"; }
  return "unknown";
}
std::string_view to_string(resource_kind) noexcept { return "readme"; }
std::string_view to_string(resource_format v) noexcept { return v == resource_format::plain_text ? "plain_text" : "markdown"; }
std::string_view to_string(strip_pattern_syntax) noexcept { return "posix_ere"; }
std::string_view to_string(footprint_format) noexcept { return "pkgfile_footprint/0"; }
std::string_view to_string(build_architecture v) noexcept { return v == build_architecture::native ? "native" : "legacy_32bit"; }

digest::digest(digest_algorithm a, std::string h) : algorithm_(a), hex_(std::move(h)) {
  std::transform(hex_.begin(), hex_.end(), hex_.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  const std::size_t n = algorithm_ == digest_algorithm::md5 ? 32 : 64;
  if (!valid_hex(hex_, n)) throw error(error_code::invalid_request, "invalid digest value");
}
digest_algorithm digest::algorithm() const noexcept { return algorithm_; }
const std::string& digest::hex() const noexcept { return hex_; }

package_identity::package_identity(std::string n, std::string v, std::string r)
  : name_(std::move(n)), version_(std::move(v)), release_(std::move(r)) {
  if (!valid_atom(name_) || !valid_atom(version_) || !valid_atom(release_))
    throw error(error_code::invalid_pkgfile, "invalid package identity");
}
const std::string& package_identity::name() const noexcept { return name_; }
const std::string& package_identity::version() const noexcept { return version_; }
const std::string& package_identity::release() const noexcept { return release_; }
std::string package_identity::version_release() const { return version_ + "-" + release_; }

descriptive_metadata::descriptive_metadata(std::optional<std::string> d, std::optional<std::string> u, std::optional<std::string> p, std::optional<std::string> m)
  : description_(std::move(d)), url_(std::move(u)), packager_(std::move(p)), maintainer_(std::move(m)) {
  const std::optional<std::string>* fields[] = {&description_, &url_, &packager_, &maintainer_};
  for (const auto* field : fields)
    if (*field && !valid_text(**field))
      throw error(error_code::invalid_metadata, "invalid descriptive metadata value");
}
const std::optional<std::string>& descriptive_metadata::description() const noexcept { return description_; }
const std::optional<std::string>& descriptive_metadata::url() const noexcept { return url_; }
const std::optional<std::string>& descriptive_metadata::packager() const noexcept { return packager_; }
const std::optional<std::string>& descriptive_metadata::maintainer() const noexcept { return maintainer_; }

dependency::dependency(std::string n, dependency_scope s) : name_(std::move(n)), scope_(s) {
  if (!valid_atom(name_)) throw error(error_code::invalid_metadata, "invalid dependency name: " + name_);
}
const std::string& dependency::name() const noexcept { return name_; }
dependency_scope dependency::scope() const noexcept { return scope_; }

captured_file::captured_file(std::shared_ptr<const detail::snapshot_state> s, std::filesystem::path p, digest d, std::uint32_t m, std::uintmax_t z)
 : state_(std::move(s)), relative_path_(std::move(p)), content_digest_(std::move(d)), original_mode_(m), size_(z) {}
const std::filesystem::path& captured_file::relative_path() const noexcept { return relative_path_; }
std::filesystem::path captured_file::native_path() const {
  if (!state_) throw error(error_code::invalid_request, "empty captured file");
  return state_->root / relative_path_;
}
const digest& captured_file::content_digest() const noexcept { return content_digest_; }
std::uint32_t captured_file::original_mode() const noexcept { return original_mode_; }
std::uintmax_t captured_file::size() const noexcept { return size_; }
bool captured_file::executable() const noexcept { return (original_mode_ & 0111U) != 0; }
captured_file::operator bool() const noexcept { return static_cast<bool>(state_); }

source_input::source_input(std::string d, source_input_kind k, std::string n, std::optional<std::string> l, std::vector<digest> g, std::optional<captured_file> f)
 : declaration_(std::move(d)), kind_(k), local_name_(std::move(n)), locator_(std::move(l)), digests_(std::move(g)), local_file_(std::move(f)) {
  if (!valid_text(declaration_) || !safe_basename(local_name_) || digests_.empty())
    throw error(error_code::invalid_pkgfile, "invalid source input");
  std::set<digest_algorithm> algorithms;
  for (const auto& item : digests_)
    if (!algorithms.insert(item.algorithm()).second)
      throw error(error_code::invalid_pkgfile, "duplicate source digest algorithm");
  if (kind_ == source_input_kind::remote) {
    if (!locator_ || *locator_ != declaration_ || local_file_)
      throw error(error_code::invalid_pkgfile, "invalid remote source input");
  } else if (locator_ || !local_file_ ||
             local_file_->relative_path().filename().string() != local_name_) {
    throw error(error_code::invalid_pkgfile, "invalid recipe-local source input");
  }
}
const std::string& source_input::declaration() const noexcept { return declaration_; }
source_input_kind source_input::kind() const noexcept { return kind_; }
const std::string& source_input::local_name() const noexcept { return local_name_; }
const std::optional<std::string>& source_input::locator() const noexcept { return locator_; }
const std::vector<digest>& source_input::digests() const noexcept { return digests_; }
const std::optional<captured_file>& source_input::local_file() const noexcept { return local_file_; }

recipe_descriptor::recipe_descriptor(source_format f, recipe_entry_point e, captured_file p) : format_(f), entry_point_(e), program_(std::move(p)) {
  if (!program_) throw error(error_code::invalid_request, "recipe has no captured program");
}
source_format recipe_descriptor::format() const noexcept { return format_; }
recipe_entry_point recipe_descriptor::entry_point() const noexcept { return entry_point_; }
const captured_file& recipe_descriptor::program() const noexcept { return program_; }

lifecycle_action::lifecycle_action(lifecycle_phase p, captured_file f) : phase_(p), program_(std::move(f)) {
  if (!program_) throw error(error_code::invalid_request, "lifecycle action has no captured program");
}
lifecycle_phase lifecycle_action::phase() const noexcept { return phase_; }
const captured_file& lifecycle_action::program() const noexcept { return program_; }

resource::resource(resource_kind k, resource_format f, captured_file file) : kind_(k), format_(f), file_(std::move(file)) {
  if (!file_) throw error(error_code::invalid_request, "resource has no captured file");
}
resource_kind resource::kind() const noexcept { return kind_; }
resource_format resource::format() const noexcept { return format_; }
const captured_file& resource::file() const noexcept { return file_; }

strip_exclusion::strip_exclusion(strip_pattern_syntax s, std::string p) : syntax_(s), pattern_(std::move(p)) {
  if (pattern_.find('\0') != std::string::npos)
    throw error(error_code::invalid_sidecar, "strip pattern contains NUL");
}
strip_pattern_syntax strip_exclusion::syntax() const noexcept { return syntax_; }
const std::string& strip_exclusion::pattern() const noexcept { return pattern_; }

footprint_declaration::footprint_declaration(footprint_format f, captured_file file) : format_(f), file_(std::move(file)) {
  if (!file_) throw error(error_code::invalid_request, "footprint has no captured file");
}
footprint_format footprint_declaration::format() const noexcept { return format_; }
const captured_file& footprint_declaration::file() const noexcept { return file_; }

build_description::build_description(package_identity i, descriptive_metadata m, std::vector<dependency> d, std::vector<source_input> s, recipe_descriptor r,
 std::vector<lifecycle_action> a, std::vector<resource> res, std::vector<strip_exclusion> x, std::optional<footprint_declaration> f, build_architecture arch)
 : identity_(std::move(i)), metadata_(std::move(m)), dependencies_(std::move(d)), sources_(std::move(s)), recipe_(std::move(r)), lifecycle_actions_(std::move(a)), resources_(std::move(res)), strip_exclusions_(std::move(x)), footprint_(std::move(f)), architecture_(arch) {
  std::set<std::string> dependency_names;
  for (const auto& item : dependencies_)
    if (!dependency_names.insert(item.name()).second)
      throw error(error_code::invalid_metadata, "duplicate normalized dependency");
  std::set<std::string> source_names;
  for (const auto& item : sources_)
    if (!source_names.insert(item.local_name()).second)
      throw error(error_code::invalid_pkgfile, "duplicate normalized source name");
  std::set<lifecycle_phase> phases;
  for (const auto& item : lifecycle_actions_)
    if (!phases.insert(item.phase()).second)
      throw error(error_code::invalid_sidecar, "duplicate lifecycle phase");
  std::set<std::pair<resource_kind, resource_format>> resource_keys;
  for (const auto& item : resources_)
    if (!resource_keys.emplace(item.kind(), item.format()).second)
      throw error(error_code::invalid_sidecar, "duplicate source resource");
}
const package_identity& build_description::identity() const noexcept { return identity_; }
const descriptive_metadata& build_description::metadata() const noexcept { return metadata_; }
const std::vector<dependency>& build_description::dependencies() const noexcept { return dependencies_; }
const std::vector<source_input>& build_description::sources() const noexcept { return sources_; }
const recipe_descriptor& build_description::recipe() const noexcept { return recipe_; }
const std::vector<lifecycle_action>& build_description::lifecycle_actions() const noexcept { return lifecycle_actions_; }
const std::vector<resource>& build_description::resources() const noexcept { return resources_; }
const std::vector<strip_exclusion>& build_description::strip_exclusions() const noexcept { return strip_exclusions_; }
const std::optional<footprint_declaration>& build_description::footprint() const noexcept { return footprint_; }
build_architecture build_description::architecture() const noexcept { return architecture_; }

} // namespace pkgsource
