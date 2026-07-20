// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file snapshot.h
 *  \brief Lifetime-owned immutable package-source captures.
 */
#pragma once

#include <filesystem>
#include <memory>

#include <libpkgsource/model.h>

namespace pkgsource {

/*! \brief Caller-supplied location of one package-source directory. */
class source_location final {
public:
  /*! \throws error when \a path is empty. */
  explicit source_location(std::filesystem::path path);
  [[nodiscard]] const std::filesystem::path& path() const noexcept;
private:
  std::filesystem::path path_;
};

/*! \brief Immutable normalized result and owner of its private captured tree.
 *
 * The fingerprint identifies the captured source-protocol contents.  File
 * descriptors are not kept open; native_root() exists for controlled adapters
 * and must be treated as read-only.
 */
class source_snapshot final {
public:
  source_snapshot() = delete;
  [[nodiscard]] const source_location& origin() const noexcept;
  [[nodiscard]] source_format format() const noexcept;
  [[nodiscard]] const digest& fingerprint() const noexcept;
  [[nodiscard]] const build_description& build() const noexcept;
  /*! \brief Return the private captured source root.
   *  \throws error only for an invalid moved-from implementation state.
   */
  [[nodiscard]] std::filesystem::path native_root() const;
private:
  friend class pkgfile_backend;
  source_snapshot(source_location origin, source_format format,
                  digest fingerprint, build_description build,
                  std::shared_ptr<const detail::snapshot_state> state);
  source_location origin_;
  source_format format_;
  digest fingerprint_;
  build_description build_;
  std::shared_ptr<const detail::snapshot_state> state_;
};

} // namespace pkgsource
