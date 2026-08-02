// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/** @file codec.h
 *  @brief Canonical durable records for sealed package-source authority.
 */
#pragma once

#include <libpkgsource-codec/export.h>
#include <libpkgsource/snapshot.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

/** @brief Durable record authority owned by libpkgsource. */
namespace pkgsource::codec {

/** @brief Supported profile-catalog record schema version. */
inline constexpr std::uint16_t profile_catalog_encoding_version = 1;
/** @brief Supported source-snapshot record schema version. */
inline constexpr std::uint16_t source_snapshot_encoding_version = 1;
/** @brief Maximum accepted encoded profile-catalog size in bytes. */
inline constexpr std::size_t maximum_profile_catalog_encoding_size =
    64U * 1024U * 1024U;
/** @brief Maximum accepted encoded source-snapshot size in bytes. */
inline constexpr std::size_t maximum_source_snapshot_encoding_size =
    128U * 1024U * 1024U;
/** @brief Maximum number of items accepted in any bounded record collection. */
inline constexpr std::uint32_t maximum_record_item_count = 1'000'000U;

/** @brief Stable durable-record failure categories. */
enum class codec_error_code : std::uint8_t {
  size_limit = 1,    ///< Envelope, string, or collection exceeds a bound.
  truncated = 2,     ///< Required bytes end before the record is complete.
  invalid_magic = 3, ///< The record type magic is not recognized.
  unsupported_version = 4, ///< The schema version is not supported.
  checksum_mismatch = 5,   ///< Whole-record SHA-256 verification failed.
  invalid_record = 6,      ///< Field tag, value, or owner material is invalid.
  identity_mismatch = 7, ///< Stored semantic identity disagrees with resealing.
  noncanonical = 8,      ///< Valid material does not use canonical byte form.
};

/** @brief Exception carrying a stable durable-record failure category. */
class PKGSOURCE_CODEC_API codec_error final : public std::invalid_argument {
public:
  /** Construct a typed codec failure.
   * @param code Stable machine-readable category.
   * @param message Human-readable diagnostic text.
   */
  codec_error(codec_error_code code, std::string message);

  /** Destroy the polymorphic exception value. */
  ~codec_error() override;

  /** Return the stable failure category.
   * @return Category supplied at construction.
   */
  [[nodiscard]] codec_error_code code() const noexcept;

private:
  codec_error_code code_;
};

/** @brief Canonical bytes of one complete profile-catalog record. */
using profile_catalog_encoding = std::vector<std::uint8_t>;
/** @brief Canonical bytes of one complete source-snapshot record. */
using source_snapshot_encoding = std::vector<std::uint8_t>;

/** Encode one complete sealed profile universe canonically.
 * @param catalog Sealed owner authority to encode.
 * @return Canonical bounded profile-catalog record.
 * @throws codec_error with codec_error_code::size_limit when the result would
 *         exceed a protocol bound.
 */
[[nodiscard]] PKGSOURCE_CODEC_API profile_catalog_encoding
encode_profile_catalog(const profile_catalog& catalog);

/** Decode and reseal one profile-catalog record.
 * @param encoding Complete record bytes.
 * @return Reconstructed profile catalog produced by profile_catalog::seal().
 * @throws codec_error for envelope, checksum, record, identity, or canonicality
 *         failure.
 */
[[nodiscard]] PKGSOURCE_CODEC_API profile_catalog
decode_profile_catalog(const profile_catalog_encoding& encoding);

/** Encode one source snapshot with its exact retained profile closure.
 * @param snapshot Sealed owner authority to encode.
 * @return Canonical bounded source-snapshot record.
 * @throws codec_error with codec_error_code::size_limit when the result would
 *         exceed a protocol bound.
 */
[[nodiscard]] PKGSOURCE_CODEC_API source_snapshot_encoding
encode_source_snapshot(const source_snapshot& snapshot);

/** Decode and reseal one source-snapshot record.
 * @param encoding Complete record bytes.
 * @return Reconstructed snapshot produced through profile and source sealers.
 * @throws codec_error for envelope, checksum, record, identity, or canonicality
 *         failure.
 */
[[nodiscard]] PKGSOURCE_CODEC_API source_snapshot
decode_source_snapshot(const source_snapshot_encoding& encoding);

} // namespace pkgsource::codec
