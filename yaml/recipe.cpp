// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "document.h"

#include <charconv>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pkgsource::yaml_adapter {
namespace {

using namespace detail;

std::uint32_t release_value(const node& value, const source_origin& origin,
                            const std::string& path)
{
  const std::string& text = scalar_value(value, origin, path);
  if (text.empty() || (text.size() > 1 && text.front() == '0'))
    fail(yaml_error_code::invalid_value, origin, path, value.mark,
         "release must be a canonical positive decimal integer");
  std::uint64_t parsed = 0;
  const auto result = std::from_chars(text.data(), text.data() + text.size(),
                                      parsed, 10);
  if (result.ec != std::errc() || result.ptr != text.data() + text.size()
      || parsed == 0
      || parsed > std::numeric_limits<std::uint32_t>::max())
    fail(yaml_error_code::invalid_value, origin, path, value.mark,
         "release must be a canonical positive decimal integer");
  return static_cast<std::uint32_t>(parsed);
}

lifecycle_action action_value(std::string_view text,
                              const source_origin& origin,
                              const std::string& path,
                              const node& value)
{
  if (text == "pre-install") return lifecycle_action::pre_install;
  if (text == "post-install") return lifecycle_action::post_install;
  if (text == "pre-remove") return lifecycle_action::pre_remove;
  if (text == "post-remove") return lifecycle_action::post_remove;
  fail(yaml_error_code::invalid_value, origin, path, value.mark,
       "invalid lifecycle action: " + std::string(text));
}

program program_value(const node& value, const source_origin& origin,
                      const std::string& path)
{
  allow_keys(value, origin, path, {"language", "script"});
  const node& language_node = required_key(value, "language", origin, path);
  const node& script_node = required_key(value, "script", origin, path);
  const std::string language_path = child_path(path, "language");
  const std::string script_path = child_path(path, "script");
  if (scalar_value(language_node, origin, language_path) != "posix-shell")
    fail(yaml_error_code::invalid_value, origin, language_path,
         language_node.mark, "language must be exactly posix-shell");
  const std::string material = scalar_value(script_node, origin, script_path);
  return semantic_value(origin, script_path, script_node, [&] {
    return program(program_language::posix_shell, material);
  });
}

std::vector<requirement_declaration> requirement_sequence(
    const node& value, const source_origin& origin, const std::string& path,
    const requirement_scope& scope)
{
  const auto& entries = sequence_value(value, origin, path);
  std::vector<requirement_declaration> result;
  result.reserve(entries.size());
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const std::string item_path = path + "[" + std::to_string(i) + "]";
    requirement_subject subject = subject_value(entries[i], origin, item_path);
    result.emplace_back(scope, std::move(subject),
                        provenance(origin, item_path, entries[i]));
  }
  return result;
}

void append_requirements(std::vector<requirement_declaration>& target,
                         std::vector<requirement_declaration> values)
{
  target.insert(target.end(),
                std::make_move_iterator(values.begin()),
                std::make_move_iterator(values.end()));
}

std::vector<requirement_declaration> requirements_value(
    const node& value, const source_origin& origin,
    const std::string& path)
{
  allow_keys(value, origin, path, {"build", "run", "check", "lifecycle"});
  std::vector<requirement_declaration> result;
  if (const node* build = find_key(value, "build"))
    append_requirements(result, requirement_sequence(
        *build, origin, child_path(path, "build"), requirement_scope::build()));
  if (const node* run = find_key(value, "run"))
    append_requirements(result, requirement_sequence(
        *run, origin, child_path(path, "run"), requirement_scope::run()));
  if (const node* check = find_key(value, "check"))
    append_requirements(result, requirement_sequence(
        *check, origin, child_path(path, "check"), requirement_scope::check()));
  if (const node* lifecycle = find_key(value, "lifecycle")) {
    const std::string lifecycle_path = child_path(path, "lifecycle");
    require_kind(*lifecycle, node_kind::mapping, origin, lifecycle_path,
                 lifecycle_path);
    for (std::size_t i = 0; i < lifecycle->children.size(); i += 2) {
      const node& key = lifecycle->children[i];
      const node& subjects = lifecycle->children[i + 1];
      const std::string action_path = child_path(lifecycle_path, key.scalar);
      const lifecycle_action action = action_value(
          key.scalar, origin, action_path, key);
      append_requirements(result, requirement_sequence(
          subjects, origin, action_path,
          requirement_scope::lifecycle(action)));
    }
  }
  return result;
}

std::vector<lifecycle_program> lifecycle_value(
    const node& value, const source_origin& origin,
    const std::string& path)
{
  require_kind(value, node_kind::mapping, origin, path, path);
  std::vector<lifecycle_program> result;
  result.reserve(value.children.size() / 2);
  for (std::size_t i = 0; i < value.children.size(); i += 2) {
    const node& key = value.children[i];
    const node& program_node = value.children[i + 1];
    const std::string action_path = child_path(path, key.scalar);
    const lifecycle_action action = action_value(
        key.scalar, origin, action_path, key);
    result.emplace_back(action,
                        program_value(program_node, origin, action_path));
  }
  return result;
}

std::vector<architecture_reference> architecture_sequence(
    const node& value, const source_origin& origin,
    const std::string& path)
{
  const auto& entries = sequence_value(value, origin, path);
  std::vector<architecture_reference> result;
  result.reserve(entries.size());
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const std::string item_path = path + "[" + std::to_string(i) + "]";
    const std::string name = scalar_value(entries[i], origin, item_path);
    result.push_back(semantic_value(origin, item_path, entries[i], [&] {
      return architecture_reference(name);
    }));
  }
  return result;
}

architecture_requirements architectures_value(
    const node* value, const source_origin& origin)
{
  if (!value)
    return architecture_requirements({}, {});
  const std::string path = "architectures";
  allow_keys(*value, origin, path, {"build", "target"});
  std::vector<architecture_reference> build;
  std::vector<architecture_reference> target;
  if (const node* entries = find_key(*value, "build"))
    build = architecture_sequence(*entries, origin, "architectures.build");
  if (const node* entries = find_key(*value, "target"))
    target = architecture_sequence(*entries, origin, "architectures.target");
  return semantic_value(origin, path, *value, [&] {
    return architecture_requirements(std::move(build), std::move(target));
  });
}

std::vector<source_input> sources_value(const node& value,
                                        const source_origin& origin,
                                        const std::string& path)
{
  const auto& entries = sequence_value(value, origin, path);
  std::vector<source_input> result;
  result.reserve(entries.size());
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const node& entry = entries[i];
    const std::string item_path = path + "[" + std::to_string(i) + "]";
    allow_keys(entry, origin, item_path, {"url", "path", "name", "sha256"});
    const node* url = find_key(entry, "url");
    const node* local = find_key(entry, "path");
    if ((url != nullptr) == (local != nullptr))
      fail(yaml_error_code::invalid_value, origin, item_path, entry.mark,
           "source must contain exactly one of url or path");
    const node& name_node = required_key(entry, "name", origin, item_path);
    const node& digest_node = required_key(entry, "sha256", origin, item_path);
    const std::string name_path = child_path(item_path, "name");
    const std::string digest_path = child_path(item_path, "sha256");
    const std::string name = scalar_value(name_node, origin, name_path);
    const std::string hash = scalar_value(digest_node, origin, digest_path);
    digest content = semantic_value(origin, digest_path, digest_node, [&] {
      return digest(digest_algorithm::sha256, hash);
    });
    if (url) {
      const std::string locator_path = child_path(item_path, "url");
      const std::string locator = scalar_value(*url, origin, locator_path);
      result.push_back(semantic_value(origin, item_path, entry, [&] {
        return source_input::remote(locator, name, content);
      }));
    } else {
      const std::string locator_path = child_path(item_path, "path");
      const std::string locator = scalar_value(*local, origin, locator_path);
      result.push_back(semantic_value(origin, item_path, entry, [&] {
        return source_input::local(locator, name, content);
      }));
    }
  }
  return result;
}

package_release package_release_value(const node& package,
                                      const source_origin& origin)
{
  const std::string path = "package";
  allow_keys(package, origin, path,
             {"name", "version", "release", "summary", "description",
              "homepage", "licenses"});
  const node& name_node = required_key(package, "name", origin, path);
  const node& version_node = required_key(package, "version", origin, path);
  const node& release_node = required_key(package, "release", origin, path);
  const std::string name = scalar_value(name_node, origin, "package.name");
  const std::string version = scalar_value(
      version_node, origin, "package.version");
  const std::uint32_t release = release_value(
      release_node, origin, "package.release");
  return semantic_value(origin, path, package, [&] {
    return package_release(package_reference(name), version, release);
  });
}

package_metadata package_metadata_value(const node& package,
                                        const source_origin& origin)
{
  const node& summary_node = required_key(package, "summary", origin, "package");
  const node& licenses_node = required_key(package, "licenses", origin, "package");
  const std::string summary = scalar_value(
      summary_node, origin, "package.summary");
  std::optional<std::string> description;
  std::optional<std::string> homepage;
  if (const node* value = find_key(package, "description"))
    description = scalar_value(*value, origin, "package.description");
  if (const node* value = find_key(package, "homepage"))
    homepage = scalar_value(*value, origin, "package.homepage");
  const auto& license_nodes = sequence_value(
      licenses_node, origin, "package.licenses");
  std::vector<std::string> licenses;
  licenses.reserve(license_nodes.size());
  for (std::size_t i = 0; i < license_nodes.size(); ++i)
    licenses.push_back(scalar_value(
        license_nodes[i], origin,
        "package.licenses[" + std::to_string(i) + "]"));
  return semantic_value(origin, "package", package, [&] {
    return package_metadata(summary, description, homepage, std::move(licenses));
  });
}


} // namespace

parsed_recipe_document::parsed_recipe_document(
    source_origin origin, recipe_declaration declaration)
    : origin_(std::move(origin)), declaration_(std::move(declaration))
{
}
const source_origin& parsed_recipe_document::origin() const noexcept
{
  return origin_;
}
const recipe_declaration& parsed_recipe_document::declaration() const noexcept
{
  return declaration_;
}

namespace {

parsed_recipe_document parse_recipe_yaml(
    std::string_view bytes, source_origin origin,
    std::string_view expected_format, bool check_program_allowed)
{
  node root = parse_document(bytes, origin);
  if (check_program_allowed)
    allow_keys(root, origin, "$",
               {"format", "package", "requirements", "sources", "build",
                "check", "lifecycle", "architectures"});
  else
    allow_keys(root, origin, "$",
               {"format", "package", "requirements", "sources", "build",
                "lifecycle", "architectures"});
  require_format(root, origin, expected_format);

  const node& package = required_key(root, "package", origin, "$");
  require_kind(package, node_kind::mapping, origin, "package", "package");
  package_release release = package_release_value(package, origin);
  package_metadata metadata = package_metadata_value(package, origin);

  const node& requirements = required_key(root, "requirements", origin, "$");
  const node& sources = required_key(root, "sources", origin, "$");
  const node& build = required_key(root, "build", origin, "$");

  std::optional<program> check;
  if (const node* value = find_key(root, "check"))
    check = program_value(*value, origin, "check");

  std::vector<lifecycle_program> lifecycle;
  if (const node* value = find_key(root, "lifecycle"))
    lifecycle = lifecycle_value(*value, origin, "lifecycle");

  recipe_declaration declaration(
      std::move(release), std::move(metadata),
      sources_value(sources, origin, "sources"),
      program_value(build, origin, "build"),
      requirements_value(requirements, origin, "requirements"),
      std::move(lifecycle),
      architectures_value(find_key(root, "architectures"), origin),
      provenance(origin, "document", root), std::move(check));
  return parsed_recipe_document(std::move(origin), std::move(declaration));
}

} // namespace

parsed_recipe_document parse_recipe_yaml_v1(
    std::string_view bytes, source_origin origin)
{
  return parse_recipe_yaml(bytes, std::move(origin),
                           "zeppe-lin.recipe/1", false);
}

parsed_recipe_document parse_recipe_yaml_v2(
    std::string_view bytes, source_origin origin)
{
  return parse_recipe_yaml(bytes, std::move(origin),
                           "zeppe-lin.recipe/2", true);
}

source_snapshot seal_recipe_yaml_v1(std::string_view bytes,
                                    source_origin origin,
                                    const profile_catalog& profiles)
{
  parsed_recipe_document parsed = parse_recipe_yaml_v1(bytes, origin);
  return seal_source(std::move(origin), source_syntax::recipe_yaml_v1,
                     parsed.declaration(), profiles);
}

source_snapshot seal_recipe_yaml_v2(std::string_view bytes,
                                    source_origin origin,
                                    const profile_catalog& profiles)
{
  parsed_recipe_document parsed = parse_recipe_yaml_v2(bytes, origin);
  return seal_source(std::move(origin), source_syntax::recipe_yaml_v2,
                     parsed.declaration(), profiles);
}


} // namespace pkgsource::yaml_adapter
