// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file pkgfile_backend.h
 *  \brief Complete pkgfile/0 source-directory backend.
 */
#pragma once

#include <filesystem>

#include <libpkgsource/backend.h>

namespace pkgsource {

/*! \brief Inspect the complete legacy Pkgfile source-directory protocol.
 *
 * The backend captures the entire directory, evaluates the captured Pkgfile in
 * a private POSIX-shell worker, parses only the documented metadata-comment
 * and sidecar grammars in C++, and never executes build() or lifecycle files.
 */
class pkgfile_backend final : public source_backend {
public:
  /*! \brief Use the installed private pkgsource-pkgfile-worker. */
  pkgfile_backend();
  /*! \brief Use an explicit worker path, primarily for tests and embedding. */
  explicit pkgfile_backend(std::filesystem::path worker);
  [[nodiscard]] source_format format() const noexcept override;
  [[nodiscard]] bool probe(const source_location& location) const override;
  [[nodiscard]] source_snapshot inspect(
      const inspect_request& request) const override;
private:
  std::filesystem::path worker_;
};

} // namespace pkgsource
