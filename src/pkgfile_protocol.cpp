// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "internal.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <regex.h>
#include <set>
#include <sstream>

namespace pkgsource::detail {
namespace {

std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  return value;
}

bool begins_label(const std::string& payload, const std::string& label) {
  const auto value = lower(trim(payload));
  return value == label ||
         (value.size() > label.size() && value.rfind(label, 0) == 0 &&
          std::isspace(static_cast<unsigned char>(value[label.size()])) != 0);
}

void validate_metadata_value(const std::string& label, const std::string& value) {
  if (value.empty())
    throw error(error_code::invalid_metadata,
                "empty Pkgfile metadata field: " + label);
  if (std::any_of(value.begin(), value.end(),
                  [](unsigned char c){ return c == '\0' || (std::iscntrl(c) && c != '\t'); }))
    throw error(error_code::invalid_metadata,
                "control character in Pkgfile metadata field: " + label);
}

bool safe_relative(const std::filesystem::path& path) {
  if (path.empty() || path.is_absolute() || path != path.lexically_normal()) return false;
  for (const auto& component : path)
    if (component.empty() || component == "." || component == "..") return false;
  return true;
}

bool safe_local_name(const std::string& value) {
  const std::filesystem::path path(value);
  return safe_relative(path) && path.filename() == path;
}

std::string basename_for_remote(const std::string& locator,
                                const std::string& declaration) {
  std::string value = locator;
  const auto fragment = value.find('#');
  if (fragment != std::string::npos) value.erase(fragment);
  const auto query = value.find('?');
  if (query != std::string::npos) value.erase(query);
  while (!value.empty() && value.back() == '/') value.pop_back();
  const auto slash = value.find_last_of('/');
  std::string name = slash == std::string::npos ? value : value.substr(slash + 1);
  if (!safe_local_name(name))
    throw error(error_code::invalid_pkgfile,
                "source declaration has no safe local name: " + declaration);
  return name;
}

struct normalized_declaration final {
  source_input_kind kind;
  std::string local_name;
  std::optional<std::string> locator;
  std::optional<std::filesystem::path> local_path;
};

normalized_declaration parse_source_declaration(const std::string& declaration) {
  const auto scheme = declaration.find("://");
  const auto separator = declaration.find("::");
  const bool renamed_remote = scheme != std::string::npos &&
                              separator != std::string::npos &&
                              separator < scheme;

  if (renamed_remote) {
    const std::string local_name = declaration.substr(0, separator);
    const std::string locator = declaration.substr(separator + 2);
    if (!safe_local_name(local_name) || locator.empty() ||
        locator.find("://") == std::string::npos)
      throw error(error_code::invalid_pkgfile,
                  "invalid renamed remote source declaration: " + declaration);
    return {source_input_kind::remote, local_name, locator, std::nullopt};
  }

  if (scheme != std::string::npos)
    return {source_input_kind::remote,
            basename_for_remote(declaration, declaration),
            declaration, std::nullopt};

  const std::filesystem::path local_path(declaration);
  if (!safe_relative(local_path))
    throw error(error_code::unsafe_source_tree,
                "unsafe recipe-local source path: " + declaration);
  return {source_input_kind::recipe_local,
          local_path.filename().string(), std::nullopt, local_path};
}

std::map<std::string, digest> read_md5_manifest(
    const std::shared_ptr<const snapshot_state>& state) {
  const auto key = std::string(".md5sum");
  const auto found = state->files.find(key);
  if (found == state->files.end())
    throw error(error_code::invalid_checksum, "missing .md5sum");
  if (found->second.type != file_record::kind::regular)
    throw error(error_code::invalid_checksum, ".md5sum is not a regular file");
  std::istringstream input(read_text_file(state->root / key));
  std::map<std::string, digest> result;
  std::string line;
  std::size_t number = 0;
  while (std::getline(input, line)) {
    ++number;
    line = trim(std::move(line));
    if (line.empty() || line.front() == '#') continue;
    std::istringstream fields(line);
    std::string checksum;
    fields >> checksum;
    std::string filename;
    std::getline(fields, filename);
    filename = trim(std::move(filename));
    if (filename.empty())
      throw error(error_code::invalid_checksum,
                  "missing filename on .md5sum line " + std::to_string(number));
    if (filename.front() == '*')
      throw error(error_code::invalid_checksum,
                  "binary-mode .md5sum entry is unsupported on line " +
                      std::to_string(number));
    const std::filesystem::path path(filename);
    if (!safe_relative(path) || path.filename() != path)
      throw error(error_code::invalid_checksum,
                  "unsafe filename on .md5sum line " + std::to_string(number));
    digest parsed = [&] {
      try {
        return digest(digest_algorithm::md5, checksum);
      } catch (const error&) {
        throw error(error_code::invalid_checksum,
                    "invalid MD5 digest on .md5sum line " +
                        std::to_string(number));
      }
    }();
    if (!result.emplace(filename, std::move(parsed)).second)
      throw error(error_code::invalid_checksum,
                  "duplicate .md5sum entry for " + filename);
  }
  return result;
}

} // namespace

metadata_result parse_pkgfile_metadata(const std::filesystem::path& pkgfile) {
  std::istringstream input(read_text_file(pkgfile));
  std::optional<std::string> description;
  std::optional<std::string> url;
  std::optional<std::string> packager;
  std::optional<std::string> maintainer;
  std::vector<dependency> dependencies;
  std::set<std::string> dependency_names;
  std::set<std::string> seen;
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    std::string stripped = trim(line);
    if (stripped.empty()) continue;
    if (stripped.front() != '#') break;
    stripped.erase(stripped.begin());
    const std::string payload = trim(stripped);
    if (payload.empty()) continue;

    const auto colon = payload.find(':');
    const std::string candidate = lower(trim(
        colon == std::string::npos ? payload : payload.substr(0, colon)));
    const bool recognized = candidate == "description" || candidate == "url" ||
                            candidate == "packager" || candidate == "maintainer" ||
                            candidate == "depends on";
    if (!recognized) {
      if (colon == std::string::npos &&
          (begins_label(payload, "description") || begins_label(payload, "url") ||
           begins_label(payload, "packager") || begins_label(payload, "maintainer") ||
           begins_label(payload, "depends on")))
        throw error(error_code::invalid_metadata,
                    "malformed Pkgfile metadata field on line " +
                        std::to_string(line_number));
      continue;
    }
    if (colon == std::string::npos)
      throw error(error_code::invalid_metadata,
                  "missing ':' in Pkgfile metadata on line " +
                      std::to_string(line_number));
    const std::string value = trim(payload.substr(colon + 1));
    validate_metadata_value(candidate, value);
    if (!seen.insert(candidate).second)
      throw error(error_code::invalid_metadata,
                  "duplicate Pkgfile metadata field: " + candidate);
    if (candidate == "description") description = value;
    else if (candidate == "url") url = value;
    else if (candidate == "packager") packager = value;
    else if (candidate == "maintainer") maintainer = value;
    else {
      std::istringstream names(value);
      std::string name;
      while (names >> name) {
        if (!dependency_names.insert(name).second)
          throw error(error_code::invalid_metadata,
                      "duplicate dependency declaration: " + name);
        dependencies.emplace_back(name, dependency_scope::build_and_run);
      }
      if (dependencies.empty())
        throw error(error_code::invalid_metadata, "empty Depends on field");
    }
  }
  return metadata_result{
      descriptive_metadata(std::move(description), std::move(url),
                           std::move(packager), std::move(maintainer)),
      std::move(dependencies)};
}

std::vector<std::string> split_nul_records(const std::string& data) {
  if (data.empty() || data.back() != '\0')
    throw error(error_code::malformed_worker_record,
                "Pkgfile worker output is not NUL terminated");
  std::vector<std::string> fields;
  std::size_t offset = 0;
  while (offset < data.size()) {
    const auto end = data.find('\0', offset);
    if (end == std::string::npos)
      throw error(error_code::malformed_worker_record,
                  "Pkgfile worker output has an unterminated field");
    fields.emplace_back(data.substr(offset, end - offset));
    offset = end + 1;
  }
  return fields;
}

std::vector<source_input> normalize_sources(
    const std::vector<std::string>& declarations,
    const std::shared_ptr<const snapshot_state>& state) {
  if (declarations.empty()) {
    const auto found = state->files.find(".md5sum");
    if (found == state->files.end()) return {};
    auto manifest = read_md5_manifest(state);
    if (!manifest.empty())
      throw error(error_code::invalid_checksum,
                  ".md5sum contains entries but Pkgfile declares no sources");
    return {};
  }
  auto manifest = read_md5_manifest(state);
  std::set<std::string> local_names;
  std::vector<source_input> sources;
  sources.reserve(declarations.size());
  for (const auto& declaration : declarations) {
    if (declaration.empty() ||
        std::any_of(declaration.begin(), declaration.end(), [](unsigned char c) {
          return c == '\0' || std::iscntrl(c) != 0 || std::isspace(c) != 0;
        }))
      throw error(error_code::invalid_pkgfile, "empty or invalid source declaration");
    auto parsed = parse_source_declaration(declaration);
    if (!local_names.insert(parsed.local_name).second)
      throw error(error_code::invalid_pkgfile,
                  "duplicate normalized source name: " + parsed.local_name);
    const auto checksum = manifest.find(parsed.local_name);
    if (checksum == manifest.end())
      throw error(error_code::invalid_checksum,
                  "missing .md5sum entry for " + parsed.local_name);
    std::optional<captured_file> local_file;
    if (parsed.local_path)
      local_file = make_captured_file(state, *parsed.local_path);
    std::vector<digest> digests;
    digests.push_back(checksum->second);
    manifest.erase(checksum);
    sources.emplace_back(declaration, parsed.kind, parsed.local_name,
                         std::move(parsed.locator), std::move(digests),
                         std::move(local_file));
  }
  if (!manifest.empty())
    throw error(error_code::invalid_checksum,
                "unrelated .md5sum entry for " + manifest.begin()->first);
  return sources;
}

std::vector<strip_exclusion> parse_nostrip(
    const std::shared_ptr<const snapshot_state>& state) {
  const auto found = state->files.find(".nostrip");
  if (found == state->files.end()) return {};
  if (found->second.type != file_record::kind::regular)
    throw error(error_code::invalid_sidecar, ".nostrip is not a regular file");
  std::istringstream input(read_text_file(state->root / ".nostrip"));
  std::vector<strip_exclusion> result;
  std::string pattern;
  std::size_t line = 0;
  while (std::getline(input, pattern)) {
    ++line;
    if (!pattern.empty() && pattern.back() == '\r') pattern.pop_back();
    regex_t expression{};
    const int status = ::regcomp(&expression, pattern.c_str(), REG_NOSUB | REG_EXTENDED);
    if (status != 0) {
      char message[256]{};
      (void)::regerror(status, &expression, message, sizeof(message));
      throw error(error_code::invalid_sidecar,
                  "invalid .nostrip pattern on line " + std::to_string(line) +
                      ": " + message);
    }
    ::regfree(&expression);
    result.emplace_back(strip_pattern_syntax::posix_extended_regular_expression,
                        pattern);
  }
  return result;
}

} // namespace pkgsource::detail
