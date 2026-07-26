// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file adapter.h
 *  \brief Explicit libpkgsource to libpkgplan candidate projection.
 */
#pragma once

#include <stdexcept>
#include <string>

#include <libpkgplan/package_fact.h>
#include <libpkgsource/snapshot.h>

namespace pkgsource::plan_adapter {

/*! \brief Machine-readable failure while projecting sealed source truth. */
enum class projection_error_code {
  lifecycle_read,
  planner_fact,
};

/*! \brief Typed source-to-planner projection failure. */
class projection_error final : public std::runtime_error {
public:
  projection_error(projection_error_code code, std::string message);
  [[nodiscard]] projection_error_code code() const noexcept;
private:
  projection_error_code code_;
};

/*! \brief Lifetime-bound planner candidate issued from one source snapshot. */
class candidate_projection final {
public:
  candidate_projection(pkgsource::source_snapshot source,
                       pkgplan::candidate_package_fact candidate);

  [[nodiscard]] const pkgsource::source_snapshot& source() const noexcept;
  [[nodiscard]] const pkgsource::digest& source_fingerprint() const noexcept;
  [[nodiscard]] const pkgplan::candidate_package_fact& candidate() const noexcept;

private:
  pkgsource::source_snapshot source_;
  pkgplan::candidate_package_fact candidate_;
};

/*! \brief Project one immutable source snapshot into planner candidate truth.
 *
 * Only runtime dependency declarations, removal lifecycle programs, and the
 * source build-architecture fact enter planner control.  The returned value
 * retains the snapshot that issued those facts.
 */
[[nodiscard]] candidate_projection
project_candidate(pkgsource::source_snapshot source);

} // namespace pkgsource::plan_adapter
