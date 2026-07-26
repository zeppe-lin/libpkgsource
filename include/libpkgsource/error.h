// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file error.h
 *  \brief Typed libpkgsource failures.
 */
#pragma once

#include <stdexcept>
#include <string>

namespace pkgsource {

/*! \brief Stable failure categories exposed by the library. */
enum class error_code {
  invalid_request,
  invalid_identity,
  invalid_provenance,
  invalid_requirement,
  invalid_profile,
  duplicate_declaration,
  unknown_profile,
  profile_cycle,
  invalid_recipe,
  invalid_metadata,
  invalid_source,
  invalid_program,
  identity_failed,
};

/*! \brief Exception carrying an error_code and diagnostic text. */
class error : public std::runtime_error {
public:
  error(error_code code, std::string message);
  [[nodiscard]] error_code code() const noexcept;
private:
  error_code code_;
};

} // namespace pkgsource
