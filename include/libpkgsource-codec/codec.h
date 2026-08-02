// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file codec.h
 *  \brief Canonical durable encodings of sealed package-source authority.
 */
#pragma once

#include <libpkgsource-codec/export.h>
#include <libpkgsource/snapshot.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace pkgsource::codec {

inline constexpr std::uint16_t profile_catalog_encoding_version = 1;
inline constexpr std::uint16_t source_snapshot_encoding_version = 1;
inline constexpr std::size_t maximum_profile_catalog_encoding_size =
    64U * 1024U * 1024U;
inline constexpr std::size_t maximum_source_snapshot_encoding_size =
    128U * 1024U * 1024U;
inline constexpr std::uint32_t maximum_record_item_count = 1'000'000U;

enum class codec_error_code : std::uint8_t {
  size_limit = 1,
  truncated = 2,
  invalid_magic = 3,
  unsupported_version = 4,
  checksum_mismatch = 5,
  invalid_record = 6,
  identity_mismatch = 7,
  noncanonical = 8,
};

class PKGSOURCE_CODEC_API codec_error final : public std::invalid_argument {
public:
  codec_error(codec_error_code code, std::string message);
  ~codec_error() override;
  [[nodiscard]] codec_error_code code() const noexcept;

private:
  codec_error_code code_;
};

using profile_catalog_encoding = std::vector<std::uint8_t>;
using source_snapshot_encoding = std::vector<std::uint8_t>;

/*! \brief Encode one complete sealed profile universe canonically. */
[[nodiscard]] profile_catalog_encoding
encode_profile_catalog(const profile_catalog& catalog);

/*! \brief Decode only through profile declaration sealing. */
[[nodiscard]] profile_catalog
decode_profile_catalog(const profile_catalog_encoding& encoding);

/*! \brief Encode one sealed source snapshot and its exact retained profile
 * closure. */
[[nodiscard]] source_snapshot_encoding
encode_source_snapshot(const source_snapshot& snapshot);

/*! \brief Decode only through profile and source sealing; syntax is never
 * recorded. */
[[nodiscard]] source_snapshot
decode_source_snapshot(const source_snapshot_encoding& encoding);

} // namespace pkgsource::codec
