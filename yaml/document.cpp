// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "document.h"

#include <yaml.h>

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <utility>

namespace pkgsource::yaml_adapter {
namespace detail {
namespace {

struct token final {
  yaml_event_type_t type = YAML_NO_EVENT;
  source_mark mark{1, 1};
  std::string value;
  std::string anchor;
  std::string tag;
  bool has_directives = false;
};

std::uint32_t position(std::size_t value)
{
  if (value >= std::numeric_limits<std::uint32_t>::max())
    return std::numeric_limits<std::uint32_t>::max();
  return static_cast<std::uint32_t>(value + 1);
}

std::string copy_yaml(const yaml_char_t* value)
{
  return value ? reinterpret_cast<const char*>(value) : std::string();
}

class event_reader final {
public:
  event_reader(std::string_view bytes, std::string document)
      : document_(std::move(document))
  {
    if (!yaml_parser_initialize(&parser_))
      throw yaml_error(yaml_error_code::syntax, document_, "$", 1, 1,
                       "cannot initialize YAML parser");
    initialized_ = true;
    static constexpr unsigned char empty_input = 0;
    const auto* input = bytes.empty()
        ? &empty_input
        : reinterpret_cast<const unsigned char*>(bytes.data());
    yaml_parser_set_input_string(&parser_, input, bytes.size());
  }

  ~event_reader()
  {
    if (initialized_)
      yaml_parser_delete(&parser_);
  }

  event_reader(const event_reader&) = delete;
  event_reader& operator=(const event_reader&) = delete;

  token next()
  {
    yaml_event_t event{};
    if (!yaml_parser_parse(&parser_, &event)) {
      const source_mark mark{
          position(parser_.problem_mark.line),
          position(parser_.problem_mark.column),
      };
      const std::string problem = parser_.problem
          ? parser_.problem : "invalid YAML input";
      throw yaml_error(yaml_error_code::syntax, document_, "$", mark.line,
                       mark.column, problem);
    }

    token result;
    result.type = event.type;
    result.mark = {
        position(event.start_mark.line),
        position(event.start_mark.column),
    };
    switch (event.type) {
      case YAML_DOCUMENT_START_EVENT:
        result.has_directives = event.data.document_start.version_directive
            || event.data.document_start.tag_directives.start
                != event.data.document_start.tag_directives.end;
        break;
      case YAML_ALIAS_EVENT:
        result.anchor = copy_yaml(event.data.alias.anchor);
        break;
      case YAML_SCALAR_EVENT:
        result.value.assign(
            reinterpret_cast<const char*>(event.data.scalar.value),
            event.data.scalar.length);
        result.anchor = copy_yaml(event.data.scalar.anchor);
        result.tag = copy_yaml(event.data.scalar.tag);
        break;
      case YAML_SEQUENCE_START_EVENT:
        result.anchor = copy_yaml(event.data.sequence_start.anchor);
        result.tag = copy_yaml(event.data.sequence_start.tag);
        break;
      case YAML_MAPPING_START_EVENT:
        result.anchor = copy_yaml(event.data.mapping_start.anchor);
        result.tag = copy_yaml(event.data.mapping_start.tag);
        break;
      default:
        break;
    }
    yaml_event_delete(&event);
    return result;
  }

private:
  yaml_parser_t parser_{};
  bool initialized_ = false;
  std::string document_;
};

bool permitted_tag(node_kind kind, std::string_view tag)
{
  if (tag.empty())
    return true;
  switch (kind) {
    case node_kind::scalar:
      return tag == "tag:yaml.org,2002:str"
          || tag == "tag:yaml.org,2002:int";
    case node_kind::sequence:
      return tag == "tag:yaml.org,2002:seq";
    case node_kind::mapping:
      return tag == "tag:yaml.org,2002:map";
  }
  return false;
}

node parse_node(event_reader& reader, token first,
                const source_origin& origin, const std::string& path)
{
  if (first.type == YAML_ALIAS_EVENT)
    fail(yaml_error_code::unsupported_feature, origin, path, first.mark,
         "YAML aliases are not supported");

  node_kind kind;
  switch (first.type) {
    case YAML_SCALAR_EVENT: kind = node_kind::scalar; break;
    case YAML_SEQUENCE_START_EVENT: kind = node_kind::sequence; break;
    case YAML_MAPPING_START_EVENT: kind = node_kind::mapping; break;
    default:
      fail(yaml_error_code::invalid_document, origin, path, first.mark,
           "expected a YAML value");
  }

  if (!first.anchor.empty())
    fail(yaml_error_code::unsupported_feature, origin, path, first.mark,
         "YAML anchors are not supported");
  if (!permitted_tag(kind, first.tag))
    fail(yaml_error_code::unsupported_feature, origin, path, first.mark,
         "custom or incompatible YAML tags are not supported");

  node result{kind, first.mark, {}, {}};
  if (kind == node_kind::scalar) {
    result.scalar = std::move(first.value);
    return result;
  }

  std::size_t index = 0;
  while (true) {
    token next = reader.next();
    if ((kind == node_kind::sequence && next.type == YAML_SEQUENCE_END_EVENT)
        || (kind == node_kind::mapping && next.type == YAML_MAPPING_END_EVENT))
      break;
    if (next.type == YAML_STREAM_END_EVENT
        || next.type == YAML_DOCUMENT_END_EVENT)
      fail(yaml_error_code::syntax, origin, path, next.mark,
           "unterminated YAML collection");

    const std::string item_path = kind == node_kind::sequence
        ? path + "[" + std::to_string(index) + "]"
        : path;
    result.children.push_back(parse_node(reader, std::move(next), origin,
                                         item_path));
    ++index;
  }

  if (kind == node_kind::mapping && result.children.size() % 2 != 0)
    fail(yaml_error_code::syntax, origin, path, result.mark,
         "mapping has an incomplete key/value pair");
  return result;
}

void validate_tree(const node& value, const source_origin& origin,
                   const std::string& path)
{
  if (value.kind == node_kind::sequence) {
    for (std::size_t i = 0; i < value.children.size(); ++i)
      validate_tree(value.children[i], origin,
                    path + "[" + std::to_string(i) + "]");
    return;
  }
  if (value.kind != node_kind::mapping)
    return;

  std::set<std::string> keys;
  for (std::size_t i = 0; i < value.children.size(); i += 2) {
    const node& key = value.children[i];
    const node& child = value.children[i + 1];
    if (key.kind != node_kind::scalar)
      fail(yaml_error_code::invalid_type, origin, path, key.mark,
           "mapping keys must be scalars");
    if (key.scalar == "<<")
      fail(yaml_error_code::unsupported_feature, origin, path, key.mark,
           "YAML merge keys are not supported");
    if (!keys.insert(key.scalar).second)
      fail(yaml_error_code::duplicate_key, origin,
           child_path(path, key.scalar), key.mark,
           "duplicate mapping key: " + key.scalar);
    validate_tree(child, origin, child_path(path, key.scalar));
  }
}

} // namespace

[[noreturn]] void fail(yaml_error_code code, const source_origin& origin,
                       std::string_view path, source_mark mark,
                       std::string message)
{
  throw yaml_error(code, origin.document(), std::string(path), mark.line,
                   mark.column, std::move(message));
}

node parse_document(std::string_view bytes, const source_origin& origin)
{
  event_reader reader(bytes, origin.document());
  token event = reader.next();
  if (event.type != YAML_STREAM_START_EVENT)
    fail(yaml_error_code::syntax, origin, "$", event.mark,
         "missing YAML stream start");

  event = reader.next();
  if (event.type != YAML_DOCUMENT_START_EVENT)
    fail(yaml_error_code::invalid_document, origin, "$", event.mark,
         "expected one YAML document");
  if (event.has_directives)
    fail(yaml_error_code::unsupported_feature, origin, "$", event.mark,
         "YAML directives are not supported");

  event = reader.next();
  if (event.type == YAML_DOCUMENT_END_EVENT)
    fail(yaml_error_code::invalid_document, origin, "$", event.mark,
         "empty YAML document");
  node root = parse_node(reader, std::move(event), origin, "$");

  event = reader.next();
  if (event.type != YAML_DOCUMENT_END_EVENT)
    fail(yaml_error_code::syntax, origin, "$", event.mark,
         "expected YAML document end");
  event = reader.next();
  if (event.type == YAML_DOCUMENT_START_EVENT)
    fail(yaml_error_code::invalid_document, origin, "$", event.mark,
         "multiple YAML documents are not supported");
  if (event.type != YAML_STREAM_END_EVENT)
    fail(yaml_error_code::syntax, origin, "$", event.mark,
         "expected YAML stream end");

  validate_tree(root, origin, "$");
  return root;
}

std::string child_path(std::string_view path, std::string_view key)
{
  return path == "$" ? std::string(key)
                     : std::string(path) + "." + std::string(key);
}

const node& require_kind(const node& value, node_kind expected,
                         const source_origin& origin,
                         std::string_view path, std::string_view name)
{
  if (value.kind != expected)
    fail(yaml_error_code::invalid_type, origin, path, value.mark,
         std::string(name) + " has the wrong YAML type");
  return value;
}

const node* find_key(const node& mapping, std::string_view key)
{
  for (std::size_t i = 0; i < mapping.children.size(); i += 2)
    if (mapping.children[i].scalar == key)
      return &mapping.children[i + 1];
  return nullptr;
}

const node& required_key(const node& mapping, std::string_view key,
                         const source_origin& origin,
                         std::string_view path)
{
  if (const node* value = find_key(mapping, key))
    return *value;
  fail(yaml_error_code::missing_key, origin, child_path(path, key),
       mapping.mark, "missing required key: " + std::string(key));
}

void allow_keys(const node& mapping, const source_origin& origin,
                std::string_view path,
                std::initializer_list<std::string_view> allowed)
{
  require_kind(mapping, node_kind::mapping, origin, path, path);
  for (std::size_t i = 0; i < mapping.children.size(); i += 2) {
    const node& key = mapping.children[i];
    if (std::find(allowed.begin(), allowed.end(), key.scalar) == allowed.end())
      fail(yaml_error_code::unknown_key, origin,
           child_path(path, key.scalar), key.mark,
           "unknown key: " + key.scalar);
  }
}

const std::string& scalar_value(const node& value,
                                const source_origin& origin,
                                std::string_view path)
{
  require_kind(value, node_kind::scalar, origin, path, path);
  return value.scalar;
}

const std::vector<node>& sequence_value(const node& value,
                                        const source_origin& origin,
                                        std::string_view path)
{
  require_kind(value, node_kind::sequence, origin, path, path);
  return value.children;
}

declaration_provenance provenance(const source_origin& origin,
                                  std::string path, const node& value)
{
  return declaration_provenance(origin.document(), std::move(path),
                                value.mark.line, value.mark.column);
}

requirement_subject subject_value(const node& value,
                                  const source_origin& origin,
                                  const std::string& path)
{
  allow_keys(value, origin, path, {"package", "profile"});
  const node* package = find_key(value, "package");
  const node* profile = find_key(value, "profile");
  if ((package != nullptr) == (profile != nullptr))
    fail(yaml_error_code::invalid_value, origin, path, value.mark,
         "requirement subject must contain exactly one of package or profile");
  if (package) {
    const std::string package_path = child_path(path, "package");
    const std::string name = scalar_value(*package, origin, package_path);
    return semantic_value(origin, package_path, *package, [&] {
      return requirement_subject(package_reference(name));
    });
  }
  const std::string profile_path = child_path(path, "profile");
  const std::string name = scalar_value(*profile, origin, profile_path);
  return semantic_value(origin, profile_path, *profile, [&] {
    return requirement_subject(profile_reference(name));
  });
}

void require_format(const node& root, const source_origin& origin,
                    std::string_view expected)
{
  const node& format = required_key(root, "format", origin, "$");
  if (scalar_value(format, origin, "format") != expected)
    fail(yaml_error_code::invalid_value, origin, "format", format.mark,
         "unsupported document format; expected " + std::string(expected));
}

} // namespace detail

yaml_error::yaml_error(yaml_error_code code, std::string document,
                       std::string path, std::uint32_t line,
                       std::uint32_t column, std::string message)
    : std::runtime_error(std::move(message)), code_(code),
      document_(std::move(document)), path_(std::move(path)), line_(line),
      column_(column)
{
}

yaml_error_code yaml_error::code() const noexcept { return code_; }
const std::string& yaml_error::document() const noexcept { return document_; }
const std::string& yaml_error::path() const noexcept { return path_; }
std::uint32_t yaml_error::line() const noexcept { return line_; }
std::uint32_t yaml_error::column() const noexcept { return column_; }

} // namespace pkgsource::yaml_adapter
