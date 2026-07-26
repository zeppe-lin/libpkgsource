// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "document.h"

#include <string>
#include <utility>
#include <vector>

namespace pkgsource::yaml_adapter {

parsed_profile_document::parsed_profile_document(
    source_origin origin, std::vector<profile_declaration> declarations)
    : origin_(std::move(origin)), declarations_(std::move(declarations))
{
}

const source_origin& parsed_profile_document::origin() const noexcept
{
  return origin_;
}

const std::vector<profile_declaration>&
parsed_profile_document::declarations() const noexcept
{
  return declarations_;
}

parsed_profile_document parse_profiles_yaml_v1(
    std::string_view bytes, source_origin origin)
{
  using namespace detail;
  node root = parse_document(bytes, origin);
  allow_keys(root, origin, "$", {"format", "profiles"});
  require_format(root, origin, "zeppe-lin.profiles/1");
  const node& profiles = required_key(root, "profiles", origin, "$");
  require_kind(profiles, node_kind::mapping, origin, "profiles", "profiles");

  std::vector<profile_declaration> declarations;
  declarations.reserve(profiles.children.size() / 2);
  for (std::size_t i = 0; i < profiles.children.size(); i += 2) {
    const node& name_node = profiles.children[i];
    const node& definition = profiles.children[i + 1];
    const std::string definition_path = "profiles." + name_node.scalar;
    allow_keys(definition, origin, definition_path, {"members"});
    const node& members_node = required_key(
        definition, "members", origin, definition_path);
    const auto& member_nodes = sequence_value(
        members_node, origin, definition_path + ".members");
    std::vector<profile_member_declaration> members;
    members.reserve(member_nodes.size());
    for (std::size_t member_index = 0;
         member_index < member_nodes.size(); ++member_index) {
      const std::string member_path = definition_path + ".members["
          + std::to_string(member_index) + "]";
      requirement_subject subject = subject_value(
          member_nodes[member_index], origin, member_path);
      members.emplace_back(
          std::move(subject),
          provenance(origin, member_path, member_nodes[member_index]));
    }
    declarations.push_back(semantic_value(
        origin, definition_path, name_node, [&] {
          return profile_declaration(
              profile_reference("@" + name_node.scalar),
              provenance(origin, definition_path, name_node),
              std::move(members));
        }));
  }
  return parsed_profile_document(std::move(origin), std::move(declarations));
}

profile_catalog seal_profiles_yaml_v1(std::string_view bytes,
                                      source_origin origin)
{
  parsed_profile_document parsed = parse_profiles_yaml_v1(
      bytes, std::move(origin));
  return profile_catalog::seal(parsed.declarations());
}

} // namespace pkgsource::yaml_adapter
