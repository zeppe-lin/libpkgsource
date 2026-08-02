// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file identity.h
 *  \brief Domain-specific SHA-256 semantic identities.
 */
#pragma once

#include <string>
#include <string_view>

namespace pkgsource {

#define PKGSOURCE_DECLARE_IDENTITY(type_name)                                  \
class type_name final {                                                        \
public:                                                                        \
  [[nodiscard]] static type_name from_sha256(std::string hex);                 \
  [[nodiscard]] const std::string& hex() const noexcept;                       \
  friend bool operator==(const type_name& lhs, const type_name& rhs) noexcept; \
  friend bool operator!=(const type_name& lhs, const type_name& rhs) noexcept; \
  friend bool operator<(const type_name& lhs, const type_name& rhs) noexcept;  \
private:                                                                       \
  explicit type_name(std::string hex);                                         \
  std::string hex_;                                                            \
}

PKGSOURCE_DECLARE_IDENTITY(package_release_identity);
PKGSOURCE_DECLARE_IDENTITY(profile_identity);
PKGSOURCE_DECLARE_IDENTITY(source_snapshot_identity);

#undef PKGSOURCE_DECLARE_IDENTITY

} // namespace pkgsource
