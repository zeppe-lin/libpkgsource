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
  unsupported_format,
  unsafe_source_tree,
  unsupported_object,
  source_mutated,
  snapshot_failed,
  worker_failed,
  malformed_worker_record,
  invalid_pkgfile,
  invalid_metadata,
  invalid_checksum,
  invalid_sidecar,
  filesystem_failed,
};

/*! \brief Exception carrying an error_code and human-readable diagnostic. */
class error : public std::runtime_error {
public:
  error(error_code code, std::string message);
  [[nodiscard]] error_code code() const noexcept;
private:
  error_code code_;
};

} // namespace pkgsource
