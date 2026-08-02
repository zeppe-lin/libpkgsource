// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file model.h
 *  \brief Native parser-neutral package-source declarations and values.
 */
#pragma once

#include <libpkgsource/identity.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pkgsource {

/*! \brief Hash algorithm accepted for source content in recipe/1. */
enum class digest_algorithm {
  sha256
};
/*! \brief Requirement domain. */
enum class requirement_scope_kind {
  build,
  run,
  check,
  lifecycle
};
/*! \brief Exact package lifecycle action. */
enum class lifecycle_action {
  pre_install,
  post_install,
  pre_remove,
  post_remove,
};
/*! \brief Requirement subject domain. */
enum class requirement_subject_kind {
  package,
  profile
};
/*! \brief Declared source input location domain. */
enum class source_input_kind {
  remote,
  local
};
/*! \brief Program language understood by later execution stages. */
enum class program_language {
  posix_shell
};

[[nodiscard]] std::string_view to_string(digest_algorithm value) noexcept;
[[nodiscard]] std::string_view to_string(requirement_scope_kind value) noexcept;
[[nodiscard]] std::string_view to_string(lifecycle_action value) noexcept;
[[nodiscard]] std::string_view
to_string(requirement_subject_kind value) noexcept;
[[nodiscard]] std::string_view to_string(source_input_kind value) noexcept;
[[nodiscard]] std::string_view to_string(program_language value) noexcept;

/*! \brief Validated lowercase hexadecimal digest value. */
class digest final {
public:
  digest(digest_algorithm algorithm, std::string hex);
  [[nodiscard]] digest_algorithm algorithm() const noexcept;
  [[nodiscard]] const std::string& hex() const noexcept;
  friend bool operator==(const digest& lhs, const digest& rhs) noexcept;
  friend bool operator!=(const digest& lhs, const digest& rhs) noexcept;
  friend bool operator<(const digest& lhs, const digest& rhs) noexcept;

private:
  digest_algorithm algorithm_;
  std::string hex_;
};

/*! \brief Canonical exact package name. */
class package_reference final {
public:
  explicit package_reference(std::string name);
  [[nodiscard]] const std::string& name() const noexcept;
  friend bool operator==(const package_reference& lhs,
                         const package_reference& rhs) noexcept;
  friend bool operator!=(const package_reference& lhs,
                         const package_reference& rhs) noexcept;
  friend bool operator<(const package_reference& lhs,
                        const package_reference& rhs) noexcept;

private:
  std::string name_;
};

/*! \brief Canonical named profile reference, including its leading '@'. */
class profile_reference final {
public:
  explicit profile_reference(std::string name);
  [[nodiscard]] const std::string& name() const noexcept;
  friend bool operator==(const profile_reference& lhs,
                         const profile_reference& rhs) noexcept;
  friend bool operator!=(const profile_reference& lhs,
                         const profile_reference& rhs) noexcept;
  friend bool operator<(const profile_reference& lhs,
                        const profile_reference& rhs) noexcept;

private:
  std::string name_;
};

/*! \brief Canonical build or target architecture name. */
class architecture_reference final {
public:
  explicit architecture_reference(std::string name);
  [[nodiscard]] const std::string& name() const noexcept;
  friend bool operator==(const architecture_reference& lhs,
                         const architecture_reference& rhs) noexcept;
  friend bool operator!=(const architecture_reference& lhs,
                         const architecture_reference& rhs) noexcept;
  friend bool operator<(const architecture_reference& lhs,
                        const architecture_reference& rhs) noexcept;

private:
  std::string name_;
};

/*! \brief Exact source declaration site retained through sealing. */
class declaration_provenance final {
public:
  declaration_provenance(std::string document,
                         std::string path,
                         std::uint32_t line,
                         std::uint32_t column);
  [[nodiscard]] const std::string& document() const noexcept;
  [[nodiscard]] const std::string& path() const noexcept;
  [[nodiscard]] std::uint32_t line() const noexcept;
  [[nodiscard]] std::uint32_t column() const noexcept;
  friend bool operator==(const declaration_provenance& lhs,
                         const declaration_provenance& rhs) noexcept;
  friend bool operator!=(const declaration_provenance& lhs,
                         const declaration_provenance& rhs) noexcept;
  friend bool operator<(const declaration_provenance& lhs,
                        const declaration_provenance& rhs) noexcept;

private:
  std::string document_;
  std::string path_;
  std::uint32_t line_;
  std::uint32_t column_;
};

/*! \brief Typed requirement scope with lifecycle binding where applicable. */
class requirement_scope final {
public:
  [[nodiscard]] static requirement_scope build();
  [[nodiscard]] static requirement_scope run();
  [[nodiscard]] static requirement_scope check();
  [[nodiscard]] static requirement_scope lifecycle(lifecycle_action action);
  [[nodiscard]] requirement_scope_kind kind() const noexcept;
  [[nodiscard]] const std::optional<lifecycle_action>& action() const noexcept;
  friend bool operator==(const requirement_scope& lhs,
                         const requirement_scope& rhs) noexcept;
  friend bool operator!=(const requirement_scope& lhs,
                         const requirement_scope& rhs) noexcept;
  friend bool operator<(const requirement_scope& lhs,
                        const requirement_scope& rhs) noexcept;

private:
  requirement_scope(requirement_scope_kind kind,
                    std::optional<lifecycle_action> action);
  requirement_scope_kind kind_;
  std::optional<lifecycle_action> action_;
};

/*! \brief Exact package or profile subject. */
class requirement_subject final {
public:
  explicit requirement_subject(package_reference package);
  explicit requirement_subject(profile_reference profile);
  [[nodiscard]] requirement_subject_kind kind() const noexcept;
  [[nodiscard]] const package_reference& package() const;
  [[nodiscard]] const profile_reference& profile() const;
  [[nodiscard]] std::string text() const;
  friend bool operator==(const requirement_subject& lhs,
                         const requirement_subject& rhs) noexcept;
  friend bool operator!=(const requirement_subject& lhs,
                         const requirement_subject& rhs) noexcept;
  friend bool operator<(const requirement_subject& lhs,
                        const requirement_subject& rhs) noexcept;

private:
  requirement_subject_kind kind_;
  std::optional<package_reference> package_;
  std::optional<profile_reference> profile_;
};

/*! \brief One parser-neutral requirement declaration. */
class requirement_declaration final {
public:
  requirement_declaration(requirement_scope scope,
                          requirement_subject subject,
                          declaration_provenance provenance);
  [[nodiscard]] const requirement_scope& scope() const noexcept;
  [[nodiscard]] const requirement_subject& subject() const noexcept;
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

private:
  requirement_scope scope_;
  requirement_subject subject_;
  declaration_provenance provenance_;
};

/*! \brief Native package release coordinates and semantic identity. */
class package_release final {
public:
  package_release(package_reference package,
                  std::string version,
                  std::uint32_t release);
  [[nodiscard]] const package_reference& package() const noexcept;
  [[nodiscard]] const std::string& version() const noexcept;
  [[nodiscard]] std::uint32_t release() const noexcept;
  [[nodiscard]] const package_release_identity& identity() const noexcept;
  [[nodiscard]] std::string version_release() const;

private:
  package_reference package_;
  std::string version_;
  std::uint32_t release_;
  package_release_identity identity_;
};

/*! \brief Metadata retained for package-image and installed-state stages. */
class package_metadata final {
public:
  package_metadata(std::string summary,
                   std::optional<std::string> description,
                   std::optional<std::string> homepage,
                   std::vector<std::string> licenses);
  [[nodiscard]] const std::string& summary() const noexcept;
  [[nodiscard]] const std::optional<std::string>& description() const noexcept;
  [[nodiscard]] const std::optional<std::string>& homepage() const noexcept;
  [[nodiscard]] const std::vector<std::string>& licenses() const noexcept;

private:
  std::string summary_;
  std::optional<std::string> description_;
  std::optional<std::string> homepage_;
  std::vector<std::string> licenses_;
};

/*! \brief One normalized source input declaration. */
class source_input final {
public:
  [[nodiscard]] static source_input
  remote(std::string url, std::string local_name, digest content_digest);
  [[nodiscard]] static source_input
  local(std::string path, std::string local_name, digest content_digest);
  [[nodiscard]] source_input_kind kind() const noexcept;
  [[nodiscard]] const std::string& location() const noexcept;
  [[nodiscard]] const std::string& local_name() const noexcept;
  [[nodiscard]] const digest& content_digest() const noexcept;
  friend bool operator==(const source_input& lhs,
                         const source_input& rhs) noexcept;
  friend bool operator!=(const source_input& lhs,
                         const source_input& rhs) noexcept;
  friend bool operator<(const source_input& lhs,
                        const source_input& rhs) noexcept;

private:
  source_input(source_input_kind kind,
               std::string location,
               std::string local_name,
               digest content_digest);
  source_input_kind kind_;
  std::string location_;
  std::string local_name_;
  digest content_digest_;
};

/*! \brief Exact non-executed program bytes in the normalized model. */
class program final {
public:
  program(program_language language, std::string material);
  [[nodiscard]] program_language language() const noexcept;
  [[nodiscard]] const std::string& material() const noexcept;
  [[nodiscard]] const digest& content_digest() const noexcept;
  friend bool operator==(const program& lhs, const program& rhs) noexcept;
  friend bool operator!=(const program& lhs, const program& rhs) noexcept;
  friend bool operator<(const program& lhs, const program& rhs) noexcept;

private:
  program_language language_;
  std::string material_;
  digest content_digest_;
};

/*! \brief Program bound to one lifecycle action. */
class lifecycle_program final {
public:
  lifecycle_program(lifecycle_action action, program value);
  [[nodiscard]] lifecycle_action action() const noexcept;
  [[nodiscard]] const program& value() const noexcept;
  friend bool operator==(const lifecycle_program& lhs,
                         const lifecycle_program& rhs) noexcept;
  friend bool operator!=(const lifecycle_program& lhs,
                         const lifecycle_program& rhs) noexcept;
  friend bool operator<(const lifecycle_program& lhs,
                        const lifecycle_program& rhs) noexcept;

private:
  lifecycle_action action_;
  program value_;
};

/*! \brief Exact accepted build and target architecture sets.
 *
 * An empty set means unrestricted. Non-empty sets are sorted and unique.
 */
class architecture_requirements final {
public:
  architecture_requirements(std::vector<architecture_reference> build,
                            std::vector<architecture_reference> target);
  [[nodiscard]] const std::vector<architecture_reference>&
  build() const noexcept;
  [[nodiscard]] const std::vector<architecture_reference>&
  target() const noexcept;

private:
  std::vector<architecture_reference> build_;
  std::vector<architecture_reference> target_;
};

} // namespace pkgsource
