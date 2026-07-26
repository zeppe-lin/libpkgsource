// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgsource-yaml/parser.h>

#include <libpkgsource/error.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace pkgsource::yaml_adapter::detail {

struct source_mark final {
  std::uint32_t line;
  std::uint32_t column;
};

enum class node_kind { scalar, sequence, mapping };

struct node final {
  node_kind kind;
  source_mark mark;
  std::string scalar;
  std::vector<node> children;
};

[[noreturn]] void fail(yaml_error_code code, const source_origin& origin,
                       std::string_view path, source_mark mark,
                       std::string message);

[[nodiscard]] node parse_document(std::string_view bytes,
                                  const source_origin& origin);
[[nodiscard]] std::string child_path(std::string_view path,
                                     std::string_view key);
const node& require_kind(const node& value, node_kind expected,
                                       const source_origin& origin,
                                       std::string_view path,
                                       std::string_view name);
[[nodiscard]] const node* find_key(const node& mapping, std::string_view key);
[[nodiscard]] const node& required_key(const node& mapping,
                                       std::string_view key,
                                       const source_origin& origin,
                                       std::string_view path);
void allow_keys(const node& mapping, const source_origin& origin,
                std::string_view path,
                std::initializer_list<std::string_view> allowed);
[[nodiscard]] const std::string& scalar_value(const node& value,
                                              const source_origin& origin,
                                              std::string_view path);
[[nodiscard]] const std::vector<node>& sequence_value(
    const node& value, const source_origin& origin, std::string_view path);
[[nodiscard]] declaration_provenance provenance(const source_origin& origin,
                                                std::string path,
                                                const node& value);
[[nodiscard]] requirement_subject subject_value(const node& value,
                                                const source_origin& origin,
                                                const std::string& path);
void require_format(const node& root, const source_origin& origin,
                    std::string_view expected);

template <typename Function>
auto semantic_value(const source_origin& origin, const std::string& path,
                    const node& value, Function&& function)
    -> decltype(function())
{
  try {
    return function();
  } catch (const pkgsource::error& failure) {
    fail(yaml_error_code::invalid_value, origin, path, value.mark,
         failure.what());
  }
}

} // namespace pkgsource::yaml_adapter::detail
