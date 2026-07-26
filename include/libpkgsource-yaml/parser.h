// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file parser.h
 *  \brief Strict profiles.yml/1 syntax adapter.
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <libpkgsource/snapshot.h>

namespace pkgsource::yaml_adapter {

/*! \brief Stable YAML frontend failure categories. */
enum class yaml_error_code {
  syntax,
  unsupported_feature,
  invalid_document,
  duplicate_key,
  unknown_key,
  missing_key,
  invalid_type,
  invalid_value,
};

/*! \brief Structured syntax error with exact diagnostic provenance. */
class yaml_error final : public std::runtime_error {
public:
  yaml_error(yaml_error_code code, std::string document, std::string path,
             std::uint32_t line, std::uint32_t column, std::string message);
  [[nodiscard]] yaml_error_code code() const noexcept;
  [[nodiscard]] const std::string& document() const noexcept;
  [[nodiscard]] const std::string& path() const noexcept;
  [[nodiscard]] std::uint32_t line() const noexcept;
  [[nodiscard]] std::uint32_t column() const noexcept;
private:
  yaml_error_code code_;
  std::string document_;
  std::string path_;
  std::uint32_t line_;
  std::uint32_t column_;
};

/*! \brief Parser-neutral profile declarations from one profiles.yml/1 document. */
class parsed_profile_document final {
public:
  parsed_profile_document(source_origin origin,
                          std::vector<profile_declaration> declarations);
  [[nodiscard]] const source_origin& origin() const noexcept;
  [[nodiscard]] const std::vector<profile_declaration>&
  declarations() const noexcept;
private:
  source_origin origin_;
  std::vector<profile_declaration> declarations_;
};

/*! \brief Parse one strict profiles.yml/1 document without sealing it. */
[[nodiscard]] parsed_profile_document parse_profiles_yaml_v1(
    std::string_view bytes, source_origin origin);

/*! \brief Parse and seal one profiles.yml/1 document. */
[[nodiscard]] profile_catalog seal_profiles_yaml_v1(
    std::string_view bytes, source_origin origin);

} // namespace pkgsource::yaml_adapter
