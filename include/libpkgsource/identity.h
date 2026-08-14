// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/** \file identity.h
 *  \brief Domain-specific SHA-256 semantic identity values.
 */

#pragma once

#include <libpkgsource/export.h>

#include <string>

namespace pkgsource {

/** \brief Semantic identity of exact package release coordinates. */
class PKGSOURCE_API package_release_identity final {
public:
  /** Import a validated SHA-256 identity.
   *
   * \param hex Exactly 64 lowercase hexadecimal characters.
   * \return Validated package-release identity.
   * \throws error with error_code::invalid_identity when \p hex is invalid.
   */
  [[nodiscard]] static package_release_identity from_sha256(std::string hex);

  /** Return the canonical lowercase hexadecimal digest.
   *
   * \return Reference valid for the lifetime of this identity.
   */
  [[nodiscard]] const std::string& hex() const noexcept;

  /** Compare package-release identities for equality.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool
  operator==(const package_release_identity& lhs,
             const package_release_identity& rhs) noexcept;
  /** Compare package-release identities for inequality.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when the values differ.
   */
  friend PKGSOURCE_API bool
  operator!=(const package_release_identity& lhs,
             const package_release_identity& rhs) noexcept;
  /** Order package-release identities by canonical digest bytes.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when \p lhs precedes \p rhs in canonical order.
   */
  friend PKGSOURCE_API bool
  operator<(const package_release_identity& lhs,
            const package_release_identity& rhs) noexcept;

private:
  explicit package_release_identity(std::string hex);
  std::string hex_;
};

/** \brief Semantic identity of one deterministically sealed profile. */
class PKGSOURCE_API profile_identity final {
public:
  /** Import a validated SHA-256 identity.
   *
   * \param hex Exactly 64 lowercase hexadecimal characters.
   * \return Validated profile identity.
   * \throws error with error_code::invalid_identity when \p hex is invalid.
   */
  [[nodiscard]] static profile_identity from_sha256(std::string hex);

  /** Return the canonical lowercase hexadecimal digest.
   *
   * \return Reference valid for the lifetime of this identity.
   */
  [[nodiscard]] const std::string& hex() const noexcept;

  /** Compare profile identities for equality.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const profile_identity& lhs,
                                       const profile_identity& rhs) noexcept;
  /** Compare profile identities for inequality.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const profile_identity& lhs,
                                       const profile_identity& rhs) noexcept;
  /** Order profile identities by canonical digest bytes.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when \p lhs precedes \p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const profile_identity& lhs,
                                      const profile_identity& rhs) noexcept;

private:
  explicit profile_identity(std::string hex);
  std::string hex_;
};

/** \brief Semantic identity of one complete sealed source snapshot. */
class PKGSOURCE_API source_snapshot_identity final {
public:
  /** Import a validated SHA-256 identity.
   *
   * \param hex Exactly 64 lowercase hexadecimal characters.
   * \return Validated source-snapshot identity.
   * \throws error with error_code::invalid_identity when \p hex is invalid.
   */
  [[nodiscard]] static source_snapshot_identity from_sha256(std::string hex);

  /** Return the canonical lowercase hexadecimal digest.
   *
   * \return Reference valid for the lifetime of this identity.
   */
  [[nodiscard]] const std::string& hex() const noexcept;

  /** Compare source-snapshot identities for equality.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool
  operator==(const source_snapshot_identity& lhs,
             const source_snapshot_identity& rhs) noexcept;
  /** Compare source-snapshot identities for inequality.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when the values differ.
   */
  friend PKGSOURCE_API bool
  operator!=(const source_snapshot_identity& lhs,
             const source_snapshot_identity& rhs) noexcept;
  /** Order source-snapshot identities by canonical digest bytes.
   * \param lhs Left comparison operand.
   * \param rhs Right comparison operand.
   * \return `true` when \p lhs precedes \p rhs in canonical order.
   */
  friend PKGSOURCE_API bool
  operator<(const source_snapshot_identity& lhs,
            const source_snapshot_identity& rhs) noexcept;

private:
  explicit source_snapshot_identity(std::string hex);
  std::string hex_;
};

} // namespace pkgsource
