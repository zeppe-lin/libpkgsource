// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file model.h
 *  \brief Immutable normalized package-source value model.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pkgsource {
class captured_file;
namespace detail {
struct snapshot_state;
captured_file make_captured_file(
    const std::shared_ptr<const snapshot_state>&,
    const std::filesystem::path&);
}

/*! \brief Source-directory protocol understood by a backend. */
enum class source_format { pkgfile_v0 };
/*! \brief Algorithm attached to a normalized digest declaration. */
enum class digest_algorithm { md5, sha256 };
/*! \brief Dependency applicability in the normalized model. */
enum class dependency_scope { build_and_run };
/*! \brief Origin of one declared build input. */
enum class source_input_kind { remote, recipe_local };
/*! \brief Callable recipe entry point declared by a source format. */
enum class recipe_entry_point { build };
/*! \brief Typed package lifecycle phase. */
enum class lifecycle_phase { pre_install, post_install, pre_remove, post_remove };
/*! \brief Kind of descriptive source resource. */
enum class resource_kind { readme };
/*! \brief Representation of a descriptive resource. */
enum class resource_format { plain_text, markdown };
/*! \brief Grammar used by a strip exclusion. */
enum class strip_pattern_syntax { posix_extended_regular_expression };
/*! \brief Representation of a captured footprint declaration. */
enum class footprint_format { pkgfile_footprint_v0 };
/*! \brief Architecture selection declared by the source protocol. */
enum class build_architecture { native, legacy_32bit };

/*! \brief Return the stable diagnostic spelling of a typed value. */
[[nodiscard]] std::string_view to_string(source_format value) noexcept;
[[nodiscard]] std::string_view to_string(digest_algorithm value) noexcept;
[[nodiscard]] std::string_view to_string(dependency_scope value) noexcept;
[[nodiscard]] std::string_view to_string(source_input_kind value) noexcept;
[[nodiscard]] std::string_view to_string(recipe_entry_point value) noexcept;
[[nodiscard]] std::string_view to_string(lifecycle_phase value) noexcept;
[[nodiscard]] std::string_view to_string(resource_kind value) noexcept;
[[nodiscard]] std::string_view to_string(resource_format value) noexcept;
[[nodiscard]] std::string_view to_string(strip_pattern_syntax value) noexcept;
[[nodiscard]] std::string_view to_string(footprint_format value) noexcept;
[[nodiscard]] std::string_view to_string(build_architecture value) noexcept;

/*! \brief Validated lowercase hexadecimal digest value. */
class digest final {
public:
  /*! \throws error when \a hex has the wrong width or non-hexadecimal data. */
  digest(digest_algorithm algorithm, std::string hex);
  [[nodiscard]] digest_algorithm algorithm() const noexcept;
  [[nodiscard]] const std::string& hex() const noexcept;
private:
  digest_algorithm algorithm_;
  std::string hex_;
};

/*! \brief Normalized package release identity. */
class package_identity final {
public:
  /*! \throws error when a field is empty, contains whitespace, controls, or '/'. */
  package_identity(std::string name, std::string version, std::string release);
  [[nodiscard]] const std::string& name() const noexcept;
  [[nodiscard]] const std::string& version() const noexcept;
  [[nodiscard]] const std::string& release() const noexcept;
  [[nodiscard]] std::string version_release() const;
private:
  std::string name_;
  std::string version_;
  std::string release_;
};

/*! \brief Optional human-facing metadata declared by a package source. */
class descriptive_metadata final {
public:
  /*! \throws error when a present field is empty or contains controls. */
  descriptive_metadata(std::optional<std::string> description,
                       std::optional<std::string> url,
                       std::optional<std::string> packager,
                       std::optional<std::string> maintainer);
  [[nodiscard]] const std::optional<std::string>& description() const noexcept;
  [[nodiscard]] const std::optional<std::string>& url() const noexcept;
  [[nodiscard]] const std::optional<std::string>& packager() const noexcept;
  [[nodiscard]] const std::optional<std::string>& maintainer() const noexcept;
private:
  std::optional<std::string> description_;
  std::optional<std::string> url_;
  std::optional<std::string> packager_;
  std::optional<std::string> maintainer_;
};

/*! \brief One normalized dependency declaration. */
class dependency final {
public:
  dependency(std::string name, dependency_scope scope);
  [[nodiscard]] const std::string& name() const noexcept;
  [[nodiscard]] dependency_scope scope() const noexcept;
private:
  std::string name_;
  dependency_scope scope_;
};

/*! \brief Lifetime-bound reference to one regular file in a source snapshot.
 *
 * Copies share ownership of the private snapshot tree.  The native path remains
 * valid until the final source_snapshot or captured_file owner is destroyed.
 */
class captured_file final {
public:
  captured_file() = default;
  [[nodiscard]] const std::filesystem::path& relative_path() const noexcept;
  /*! \brief Return the file path inside the private captured tree.
   *  \throws error when this is an empty captured_file.
   */
  [[nodiscard]] std::filesystem::path native_path() const;
  [[nodiscard]] const digest& content_digest() const noexcept;
  [[nodiscard]] std::uint32_t original_mode() const noexcept;
  [[nodiscard]] std::uintmax_t size() const noexcept;
  [[nodiscard]] bool executable() const noexcept;
  [[nodiscard]] explicit operator bool() const noexcept;
private:
  friend class pkgfile_backend;
  friend class source_snapshot;
  friend captured_file detail::make_captured_file(
      const std::shared_ptr<const detail::snapshot_state>&,
      const std::filesystem::path&);
  captured_file(std::shared_ptr<const detail::snapshot_state> state,
                std::filesystem::path relative_path,
                digest content_digest,
                std::uint32_t original_mode,
                std::uintmax_t size);
  std::shared_ptr<const detail::snapshot_state> state_;
  std::filesystem::path relative_path_;
  digest content_digest_{digest_algorithm::sha256, std::string(64, '0')};
  std::uint32_t original_mode_{0};
  std::uintmax_t size_{0};
};

/*! \brief Declared input to the build recipe.
 *
 * Remote inputs carry a locator and no captured local file.  Their declaration
 * is either the locator itself or `local_name::locator` when pkgfile/0 supplies
 * an explicit distfile name.  Recipe-local inputs carry a captured regular file
 * and no remote locator.
 */
class source_input final {
public:
  source_input(std::string declaration, source_input_kind kind,
               std::string local_name, std::optional<std::string> locator,
               std::vector<digest> digests,
               std::optional<captured_file> local_file);
  [[nodiscard]] const std::string& declaration() const noexcept;
  [[nodiscard]] source_input_kind kind() const noexcept;
  [[nodiscard]] const std::string& local_name() const noexcept;
  [[nodiscard]] const std::optional<std::string>& locator() const noexcept;
  [[nodiscard]] const std::vector<digest>& digests() const noexcept;
  [[nodiscard]] const std::optional<captured_file>& local_file() const noexcept;
private:
  std::string declaration_;
  source_input_kind kind_;
  std::string local_name_;
  std::optional<std::string> locator_;
  std::vector<digest> digests_;
  std::optional<captured_file> local_file_;
};

/*! \brief Captured program and callable entry point for a build recipe. */
class recipe_descriptor final {
public:
  recipe_descriptor(source_format format, recipe_entry_point entry_point,
                    captured_file program);
  [[nodiscard]] source_format format() const noexcept;
  [[nodiscard]] recipe_entry_point entry_point() const noexcept;
  [[nodiscard]] const captured_file& program() const noexcept;
private:
  source_format format_;
  recipe_entry_point entry_point_;
  captured_file program_;
};

/*! \brief Captured lifecycle program; the library never executes it. */
class lifecycle_action final {
public:
  lifecycle_action(lifecycle_phase phase, captured_file program);
  [[nodiscard]] lifecycle_phase phase() const noexcept;
  [[nodiscard]] const captured_file& program() const noexcept;
private:
  lifecycle_phase phase_;
  captured_file program_;
};

/*! \brief Captured descriptive resource associated with a source. */
class resource final {
public:
  resource(resource_kind kind, resource_format format, captured_file file);
  [[nodiscard]] resource_kind kind() const noexcept;
  [[nodiscard]] resource_format format() const noexcept;
  [[nodiscard]] const captured_file& file() const noexcept;
private:
  resource_kind kind_;
  resource_format format_;
  captured_file file_;
};

/*! \brief One normalized path pattern excluded from stripping. */
class strip_exclusion final {
public:
  strip_exclusion(strip_pattern_syntax syntax, std::string pattern);
  [[nodiscard]] strip_pattern_syntax syntax() const noexcept;
  [[nodiscard]] const std::string& pattern() const noexcept;
private:
  strip_pattern_syntax syntax_;
  std::string pattern_;
};

/*! \brief Strongly typed captured build-footprint declaration. */
class footprint_declaration final {
public:
  footprint_declaration(footprint_format format, captured_file file);
  [[nodiscard]] footprint_format format() const noexcept;
  [[nodiscard]] const captured_file& file() const noexcept;
private:
  footprint_format format_;
  captured_file file_;
};

/*! \brief Complete normalized meaning of one inspected package source. */
class build_description final {
public:
  build_description(package_identity identity,
                    descriptive_metadata metadata,
                    std::vector<dependency> dependencies,
                    std::vector<source_input> sources,
                    recipe_descriptor recipe,
                    std::vector<lifecycle_action> lifecycle_actions,
                    std::vector<resource> resources,
                    std::vector<strip_exclusion> strip_exclusions,
                    std::optional<footprint_declaration> footprint,
                    build_architecture architecture);
  [[nodiscard]] const package_identity& identity() const noexcept;
  [[nodiscard]] const descriptive_metadata& metadata() const noexcept;
  [[nodiscard]] const std::vector<dependency>& dependencies() const noexcept;
  [[nodiscard]] const std::vector<source_input>& sources() const noexcept;
  [[nodiscard]] const recipe_descriptor& recipe() const noexcept;
  [[nodiscard]] const std::vector<lifecycle_action>& lifecycle_actions() const noexcept;
  [[nodiscard]] const std::vector<resource>& resources() const noexcept;
  [[nodiscard]] const std::vector<strip_exclusion>& strip_exclusions() const noexcept;
  [[nodiscard]] const std::optional<footprint_declaration>& footprint() const noexcept;
  [[nodiscard]] build_architecture architecture() const noexcept;
private:
  package_identity identity_;
  descriptive_metadata metadata_;
  std::vector<dependency> dependencies_;
  std::vector<source_input> sources_;
  recipe_descriptor recipe_;
  std::vector<lifecycle_action> lifecycle_actions_;
  std::vector<resource> resources_;
  std::vector<strip_exclusion> strip_exclusions_;
  std::optional<footprint_declaration> footprint_;
  build_architecture architecture_;
};

} // namespace pkgsource
