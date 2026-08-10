// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/** @file model.h
 *  @brief Parser-neutral package-source declarations and validated values.
 */
#pragma once

#include <libpkgsource/export.h>
#include <libpkgsource/identity.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pkgsource {

/** @brief Content-digest algorithms accepted by the source model. */
enum class digest_algorithm {
  sha256, ///< SHA-256 represented by 64 lowercase hexadecimal characters.
};

/** @brief Semantic requirement domains. */
enum class requirement_scope_kind {
  build,     ///< Required while producing a package image.
  run,       ///< Required by the installed package at runtime.
  check,     ///< Required while running the optional check program.
  lifecycle, ///< Required by one exact package lifecycle action.
};

/** @brief Exact package lifecycle actions. */
enum class lifecycle_action {
  pre_install,  ///< Run before package installation.
  post_install, ///< Run after package installation.
  pre_remove,   ///< Run before package removal.
  post_remove,  ///< Run after package removal.
};

/** @brief Kinds of requirement subjects. */
enum class requirement_subject_kind {
  package, ///< One exact package reference.
  profile, ///< One named requirement profile.
};

/** @brief Kinds of declared source locations. */
enum class source_input_kind {
  remote, ///< A URL-like location containing a scheme delimiter.
  local,  ///< A safe relative path supplied by the package collection.
};

/** @brief Program languages retained by the source model. */
enum class program_language {
  posix_shell, ///< Exact POSIX shell program bytes.
};

/** Return the canonical protocol spelling of a digest algorithm.
 * @param value Algorithm to render.
 * @return Static string view with process lifetime.
 */
[[nodiscard]] PKGSOURCE_API std::string_view
to_string(digest_algorithm value) noexcept;

/** Return the canonical protocol spelling of a requirement scope kind.
 * @param value Scope kind to render.
 * @return Static string view with process lifetime.
 */
[[nodiscard]] PKGSOURCE_API std::string_view
to_string(requirement_scope_kind value) noexcept;

/** Return the canonical protocol spelling of a lifecycle action.
 * @param value Lifecycle action to render.
 * @return Static string view with process lifetime.
 */
[[nodiscard]] PKGSOURCE_API std::string_view
to_string(lifecycle_action value) noexcept;

/** Return the canonical protocol spelling of a requirement subject kind.
 * @param value Subject kind to render.
 * @return Static string view with process lifetime.
 */
[[nodiscard]] PKGSOURCE_API std::string_view
to_string(requirement_subject_kind value) noexcept;

/** Return the canonical protocol spelling of a source input kind.
 * @param value Input kind to render.
 * @return Static string view with process lifetime.
 */
[[nodiscard]] PKGSOURCE_API std::string_view
to_string(source_input_kind value) noexcept;

/** Return the canonical protocol spelling of a program language.
 * @param value Program language to render.
 * @return Static string view with process lifetime.
 */
[[nodiscard]] PKGSOURCE_API std::string_view
to_string(program_language value) noexcept;

/** @brief Validated content digest. */
class PKGSOURCE_API digest final {
public:
  /** Construct a validated content digest.
   * @param algorithm Supported digest algorithm.
   * @param hex Exactly 64 lowercase hexadecimal characters for SHA-256.
   * @throws error with error_code::invalid_identity for an unsupported
   *         algorithm or malformed digest material.
   */
  digest(digest_algorithm algorithm, std::string hex);

  /** Return the digest algorithm.
   * @return Algorithm supplied at construction.
   */
  [[nodiscard]] digest_algorithm algorithm() const noexcept;

  /** Return canonical lowercase hexadecimal material.
   * @return Reference valid for the lifetime of this digest.
   */
  [[nodiscard]] const std::string& hex() const noexcept;

  /** Compare complete digest values for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const digest& lhs,
                                       const digest& rhs) noexcept;
  /** Compare complete digest values for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const digest& lhs,
                                       const digest& rhs) noexcept;
  /** Order digest values by algorithm and hexadecimal material.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const digest& lhs,
                                      const digest& rhs) noexcept;

private:
  digest_algorithm algorithm_;
  std::string hex_;
};

/** @brief Canonical exact package name. */
class PKGSOURCE_API package_reference final {
public:
  /** Construct a package reference.
   * @param name Canonical lowercase package atom.
   * @throws error with error_code::invalid_identity when @p name is invalid.
   */
  explicit package_reference(std::string name);

  /** Return the canonical package name.
   * @return Reference valid for the lifetime of this value.
   */
  [[nodiscard]] const std::string& name() const noexcept;

  /** Compare package references for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const package_reference& lhs,
                                       const package_reference& rhs) noexcept;
  /** Compare package references for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const package_reference& lhs,
                                       const package_reference& rhs) noexcept;
  /** Order package references lexicographically by canonical name.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const package_reference& lhs,
                                      const package_reference& rhs) noexcept;

private:
  std::string name_;
};

/** @brief Canonical named profile reference including its leading `@`. */
class PKGSOURCE_API profile_reference final {
public:
  /** Construct a profile reference.
   * @param name Canonical lowercase profile atom beginning with `@`.
   * @throws error with error_code::invalid_identity when @p name is invalid.
   */
  explicit profile_reference(std::string name);

  /** Return the canonical profile name including `@`.
   * @return Reference valid for the lifetime of this value.
   */
  [[nodiscard]] const std::string& name() const noexcept;

  /** Compare profile references for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const profile_reference& lhs,
                                       const profile_reference& rhs) noexcept;
  /** Compare profile references for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const profile_reference& lhs,
                                       const profile_reference& rhs) noexcept;
  /** Order profile references lexicographically by canonical name.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const profile_reference& lhs,
                                      const profile_reference& rhs) noexcept;

private:
  std::string name_;
};

/** @brief Canonical build or target architecture name. */
class PKGSOURCE_API architecture_reference final {
public:
  /** Construct an architecture reference.
   * @param name Canonical lowercase architecture atom.
   * @throws error with error_code::invalid_identity when @p name is invalid.
   */
  explicit architecture_reference(std::string name);

  /** Return the canonical architecture name.
   * @return Reference valid for the lifetime of this value.
   */
  [[nodiscard]] const std::string& name() const noexcept;

  /** Compare architecture references for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool
  operator==(const architecture_reference& lhs,
             const architecture_reference& rhs) noexcept;
  /** Compare architecture references for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool
  operator!=(const architecture_reference& lhs,
             const architecture_reference& rhs) noexcept;
  /** Order architecture references lexicographically by canonical name.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool
  operator<(const architecture_reference& lhs,
            const architecture_reference& rhs) noexcept;

private:
  std::string name_;
};

/** @brief Exact declaration site retained as diagnostic provenance. */
class PKGSOURCE_API declaration_provenance final {
public:
  /** Construct declaration provenance.
   * @param document Non-empty single-line document identifier.
   * @param path Non-empty single-line schema or field path.
   * @param line One-based source line.
   * @param column One-based source column.
   * @throws error with error_code::invalid_provenance for unsafe text or zero
   *         coordinates.
   */
  declaration_provenance(std::string document,
                         std::string path,
                         std::uint32_t line,
                         std::uint32_t column);

  /** Return the diagnostic document identifier.
   * @return Reference valid for the lifetime of this value.
   */
  [[nodiscard]] const std::string& document() const noexcept;

  /** Return the diagnostic schema or field path.
   * @return Reference valid for the lifetime of this value.
   */
  [[nodiscard]] const std::string& path() const noexcept;

  /** Return the one-based source line.
   * @return Positive line number.
   */
  [[nodiscard]] std::uint32_t line() const noexcept;

  /** Return the one-based source column.
   * @return Positive column number.
   */
  [[nodiscard]] std::uint32_t column() const noexcept;

  /** Compare complete provenance values for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool
  operator==(const declaration_provenance& lhs,
             const declaration_provenance& rhs) noexcept;
  /** Compare complete provenance values for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool
  operator!=(const declaration_provenance& lhs,
             const declaration_provenance& rhs) noexcept;
  /** Order provenance by document, path, line, and column.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool
  operator<(const declaration_provenance& lhs,
            const declaration_provenance& rhs) noexcept;

private:
  std::string document_;
  std::string path_;
  std::uint32_t line_;
  std::uint32_t column_;
};

/** @brief Typed requirement scope with lifecycle binding when required. */
class PKGSOURCE_API requirement_scope final {
public:
  /** Construct the build requirement scope.
   * @return Build scope without a lifecycle action.
   */
  [[nodiscard]] static requirement_scope build();

  /** Construct the runtime requirement scope.
   * @return Runtime scope without a lifecycle action.
   */
  [[nodiscard]] static requirement_scope run();

  /** Construct the check requirement scope.
   * @return Check scope without a lifecycle action.
   */
  [[nodiscard]] static requirement_scope check();

  /** Construct an action-bound lifecycle requirement scope.
   * @param action Exact supported lifecycle action requiring the package.
   * @return Lifecycle scope bound to @p action.
   * @throws error with error_code::invalid_requirement when @p action is not
   *         one of the declared lifecycle actions.
   */
  [[nodiscard]] static requirement_scope lifecycle(lifecycle_action action);

  /** Return the scope kind.
   * @return Build, run, check, or lifecycle.
   */
  [[nodiscard]] requirement_scope_kind kind() const noexcept;

  /** Return the lifecycle binding.
   * @return Populated only when kind() is requirement_scope_kind::lifecycle.
   */
  [[nodiscard]] const std::optional<lifecycle_action>& action() const noexcept;

  /** Compare requirement scopes for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const requirement_scope& lhs,
                                       const requirement_scope& rhs) noexcept;
  /** Compare requirement scopes for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const requirement_scope& lhs,
                                       const requirement_scope& rhs) noexcept;
  /** Order requirement scopes by kind and lifecycle action.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const requirement_scope& lhs,
                                      const requirement_scope& rhs) noexcept;

private:
  requirement_scope(requirement_scope_kind kind,
                    std::optional<lifecycle_action> action);
  requirement_scope_kind kind_;
  std::optional<lifecycle_action> action_;
};

/** @brief Exact package or profile requirement subject. */
class PKGSOURCE_API requirement_subject final {
public:
  /** Construct a package subject.
   * @param package Exact package reference.
   */
  explicit requirement_subject(package_reference package);

  /** Construct a profile subject.
   * @param profile Exact named profile reference.
   */
  explicit requirement_subject(profile_reference profile);

  /** Return the active subject kind.
   * @return Package or profile.
   */
  [[nodiscard]] requirement_subject_kind kind() const noexcept;

  /** Return the package subject.
   * @return Reference valid for the lifetime of this value.
   * @throws error with error_code::invalid_requirement when this is a profile
   *         subject.
   */
  [[nodiscard]] const package_reference& package() const;

  /** Return the profile subject.
   * @return Reference valid for the lifetime of this value.
   * @throws error with error_code::invalid_requirement when this is a package
   *         subject.
   */
  [[nodiscard]] const profile_reference& profile() const;

  /** Return canonical subject text.
   * @return Package name or profile name including its leading `@`.
   */
  [[nodiscard]] std::string text() const;

  /** Compare requirement subjects for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const requirement_subject& lhs,
                                       const requirement_subject& rhs) noexcept;
  /** Compare requirement subjects for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const requirement_subject& lhs,
                                       const requirement_subject& rhs) noexcept;
  /** Order requirement subjects by kind and canonical text.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const requirement_subject& lhs,
                                      const requirement_subject& rhs) noexcept;

private:
  requirement_subject_kind kind_;
  std::optional<package_reference> package_;
  std::optional<profile_reference> profile_;
};

/** @brief One parser-neutral requirement declaration. */
class PKGSOURCE_API requirement_declaration final {
public:
  /** Construct a requirement declaration.
   * @param scope Semantic requirement scope.
   * @param subject Package or profile subject.
   * @param provenance Exact declaration site.
   */
  requirement_declaration(requirement_scope scope,
                          requirement_subject subject,
                          declaration_provenance provenance);

  /** Return the declared scope.
   * @return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const requirement_scope& scope() const noexcept;

  /** Return the declared subject.
   * @return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const requirement_subject& subject() const noexcept;

  /** Return declaration provenance.
   * @return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

private:
  requirement_scope scope_;
  requirement_subject subject_;
  declaration_provenance provenance_;
};

/** @brief Native package release coordinates and their semantic identity. */
class PKGSOURCE_API package_release final {
public:
  /** Construct package release coordinates.
   * @param package Exact package reference.
   * @param version Non-empty single-line version without `/`.
   * @param release Positive distribution release number.
   * @throws error with error_code::invalid_metadata for invalid coordinates.
   */
  package_release(package_reference package,
                  std::string version,
                  std::uint32_t release);

  /** Return the package reference.
   * @return Reference valid for the lifetime of this release.
   */
  [[nodiscard]] const package_reference& package() const noexcept;

  /** Return exact upstream version text.
   * @return Reference valid for the lifetime of this release.
   */
  [[nodiscard]] const std::string& version() const noexcept;

  /** Return the positive distribution release number.
   * @return Distribution release number.
   */
  [[nodiscard]] std::uint32_t release() const noexcept;

  /** Return the semantic package-release identity.
   * @return Reference valid for the lifetime of this release.
   */
  [[nodiscard]] const package_release_identity& identity() const noexcept;

  /** Return conventional version-release display text.
   * @return `version-release` using exact version text and decimal release.
   */
  [[nodiscard]] std::string version_release() const;

private:
  package_reference package_;
  std::string version_;
  std::uint32_t release_;
  package_release_identity identity_;
};

/** @brief Metadata retained for package-image and installed-state owners. */
class PKGSOURCE_API package_metadata final {
public:
  /** Construct validated package metadata.
   * @param summary Non-empty single-line summary.
   * @param description Optional non-empty text allowing tabs and newlines.
   * @param homepage Optional non-empty single-line homepage text.
   * @param licenses Non-empty unique single-line license identifiers.
   * @throws error with error_code::invalid_metadata for invalid fields.
   * @throws error with error_code::duplicate_declaration for duplicate
   * licenses.
   */
  package_metadata(std::string summary,
                   std::optional<std::string> description,
                   std::optional<std::string> homepage,
                   std::vector<std::string> licenses);

  /** Return the package summary.
   * @return Reference valid for the lifetime of this metadata.
   */
  [[nodiscard]] const std::string& summary() const noexcept;

  /** Return the optional long description.
   * @return Reference valid for the lifetime of this metadata.
   */
  [[nodiscard]] const std::optional<std::string>& description() const noexcept;

  /** Return the optional homepage.
   * @return Reference valid for the lifetime of this metadata.
   */
  [[nodiscard]] const std::optional<std::string>& homepage() const noexcept;

  /** Return sorted unique license identifiers.
   * @return Reference valid for the lifetime of this metadata.
   */
  [[nodiscard]] const std::vector<std::string>& licenses() const noexcept;

private:
  std::string summary_;
  std::optional<std::string> description_;
  std::optional<std::string> homepage_;
  std::vector<std::string> licenses_;
};

/** @brief One normalized source input declaration. */
class PKGSOURCE_API source_input final {
public:
  /** Construct a remote source input.
   * @param url Non-empty single-line location containing `://`.
   * @param local_name Safe destination basename.
   * @param content_digest Required content digest.
   * @return Validated remote input.
   * @throws error with error_code::invalid_source for invalid location data.
   */
  [[nodiscard]] static source_input
  remote(std::string url, std::string local_name, digest content_digest);

  /** Construct a collection-local source input.
   * @param path Safe non-absolute relative path without `.` or `..` segments.
   * @param local_name Safe destination basename.
   * @param content_digest Required content digest.
   * @return Validated local input.
   * @throws error with error_code::invalid_source for invalid location data.
   */
  [[nodiscard]] static source_input
  local(std::string path, std::string local_name, digest content_digest);

  /** Return the source location kind.
   * @return Remote or local.
   */
  [[nodiscard]] source_input_kind kind() const noexcept;

  /** Return exact URL or relative path text.
   * @return Reference valid for the lifetime of this input.
   */
  [[nodiscard]] const std::string& location() const noexcept;

  /** Return the safe destination basename.
   * @return Reference valid for the lifetime of this input.
   */
  [[nodiscard]] const std::string& local_name() const noexcept;

  /** Return the required content digest.
   * @return Reference valid for the lifetime of this input.
   */
  [[nodiscard]] const digest& content_digest() const noexcept;

  /** Compare complete source inputs for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const source_input& lhs,
                                       const source_input& rhs) noexcept;
  /** Compare complete source inputs for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const source_input& lhs,
                                       const source_input& rhs) noexcept;
  /** Order source inputs by kind, location, local name, and digest.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const source_input& lhs,
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

/** @brief Exact non-executed program bytes in the normalized model. */
class PKGSOURCE_API program final {
public:
  /** Construct a validated program value and compute its content digest.
   * @param language Supported declared program language.
   * @param material Non-empty exact text bytes without NUL or forbidden control
   *        characters.
   * @throws error with error_code::invalid_program for an unsupported language
   *         or invalid material.
   */
  program(program_language language, std::string material);

  /** Return the declared language.
   * @return Program language.
   */
  [[nodiscard]] program_language language() const noexcept;

  /** Return exact program bytes.
   * @return Reference valid for the lifetime of this program.
   */
  [[nodiscard]] const std::string& material() const noexcept;

  /** Return the SHA-256 digest of exact program bytes.
   * @return Reference valid for the lifetime of this program.
   */
  [[nodiscard]] const digest& content_digest() const noexcept;

  /** Compare complete program values for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const program& lhs,
                                       const program& rhs) noexcept;
  /** Compare complete program values for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const program& lhs,
                                       const program& rhs) noexcept;
  /** Order program values by language, material, and digest.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const program& lhs,
                                      const program& rhs) noexcept;

private:
  program_language language_;
  std::string material_;
  digest content_digest_;
};

/** @brief Program bound to one exact lifecycle action. */
class PKGSOURCE_API lifecycle_program final {
public:
  /** Construct an action-bound lifecycle program.
   * @param action Exact supported lifecycle action.
   * @param value Exact non-executed program value.
   * @throws error with error_code::invalid_program when @p action is not one
   *         of the declared lifecycle actions.
   */
  lifecycle_program(lifecycle_action action, program value);

  /** Return the bound lifecycle action.
   * @return Lifecycle action.
   */
  [[nodiscard]] lifecycle_action action() const noexcept;

  /** Return the exact program value.
   * @return Reference valid for the lifetime of this binding.
   */
  [[nodiscard]] const program& value() const noexcept;

  /** Compare lifecycle bindings for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const lifecycle_program& lhs,
                                       const lifecycle_program& rhs) noexcept;
  /** Compare lifecycle bindings for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const lifecycle_program& lhs,
                                       const lifecycle_program& rhs) noexcept;
  /** Order lifecycle bindings by action and program.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const lifecycle_program& lhs,
                                      const lifecycle_program& rhs) noexcept;

private:
  lifecycle_action action_;
  program value_;
};

/** @brief Exact accepted build and target architecture sets.
 *
 * Empty sets mean unrestricted. Non-empty sets are normalized into sorted,
 * unique canonical architecture references.
 */
class PKGSOURCE_API architecture_requirements final {
public:
  /** Construct normalized architecture requirements.
   * @param build Accepted build-machine architectures, or empty for any.
   * @param target Accepted target architectures, or empty for any.
   * @throws error with error_code::duplicate_declaration when either
   *         normalized set contains a duplicate.
   */
  architecture_requirements(std::vector<architecture_reference> build,
                            std::vector<architecture_reference> target);

  /** Return accepted build architectures.
   * @return Sorted unique set; empty means unrestricted.
   */
  [[nodiscard]] const std::vector<architecture_reference>&
  build() const noexcept;

  /** Return accepted target architectures.
   * @return Sorted unique set; empty means unrestricted.
   */
  [[nodiscard]] const std::vector<architecture_reference>&
  target() const noexcept;

private:
  std::vector<architecture_reference> build_;
  std::vector<architecture_reference> target_;
};

} // namespace pkgsource
