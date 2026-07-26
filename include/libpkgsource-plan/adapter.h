// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file adapter.h
 *  \brief Native libpkgsource to libpkgplan candidate projection.
 */
#pragma once

#include <stdexcept>
#include <string>

#include <libpkgplan/package_fact.h>
#include <libpkgsource/snapshot.h>

namespace pkgsource::plan_adapter {

/*! \brief Machine-readable projection failure. */
enum class projection_error_code {
  identity,
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

/*! \brief Planner candidate bound to the source snapshot that issued it. */
class candidate_projection final {
public:
  candidate_projection(pkgsource::source_snapshot source,
                       pkgplan::candidate_package_fact candidate);
  [[nodiscard]] const pkgsource::source_snapshot& source() const noexcept;
  [[nodiscard]] const pkgsource::source_snapshot_identity&
  source_identity() const noexcept;
  [[nodiscard]] const pkgplan::candidate_package_fact& candidate() const noexcept;
private:
  pkgsource::source_snapshot source_;
  pkgplan::candidate_package_fact candidate_;
};

/*! \brief Project sealed runtime and durable removal control.
 *
 * Build, check, and lifecycle requirements, selected build profiles, build
 * architecture requirements, install lifecycle programs, sources, and build
 * programs remain outside libpkgplan candidate control.
 */
[[nodiscard]] candidate_projection
project_candidate(pkgsource::source_snapshot source);

} // namespace pkgsource::plan_adapter
