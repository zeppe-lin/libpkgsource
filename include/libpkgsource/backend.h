// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file backend.h
 *  \brief Source-format probing and inspection contract.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <sys/types.h>

#include <libpkgsource/snapshot.h>

namespace pkgsource {

/*! \brief Optional operating-system identity used by the private worker. */
struct worker_identity final {
  uid_t uid{};
  gid_t gid{};
  std::string user;
  std::filesystem::path home;
};

/*! \brief Explicit execution conditions for source-format evaluation.
 *
 * The environment map is an allow-list added to the library's fixed baseline;
 * the ambient process environment is not inherited.  Unsafe loader, shell
 * startup, locale-path, and reserved variables are rejected.
 */
struct evaluation_policy final {
  std::map<std::string, std::string> environment;
  std::optional<worker_identity> identity;
  std::uint32_t file_creation_mask{0022};
};

/*! \brief Request to inspect exactly one package-source directory. */
struct inspect_request final {
  source_location location;
  /*! Parent in which the private temporary snapshot is created. */
  std::optional<std::filesystem::path> snapshot_parent;
  evaluation_policy evaluation;
};

/*! \brief Abstract backend for one source-directory protocol. */
class source_backend {
public:
  virtual ~source_backend() = default;
  [[nodiscard]] virtual source_format format() const noexcept = 0;
  /*! \brief Test whether \a location has this backend's format marker.
   *
   * Probe is intentionally shallow.  A true result does not imply that full
   * inspection will succeed.
   */
  [[nodiscard]] virtual bool probe(const source_location& location) const = 0;
  /*! \brief Capture, evaluate, validate, and normalize one source directory. */
  [[nodiscard]] virtual source_snapshot inspect(
      const inspect_request& request) const = 0;
};

} // namespace pkgsource
