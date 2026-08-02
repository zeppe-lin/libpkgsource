// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "record_checksum.h"

#include <libpkgsource-codec/codec.h>

#include <string>

namespace pkgsource::codec::internal {

pkgsource::internal::sha256_digest record_checksum(
    const std::uint8_t* data,
    std::size_t size)
{
  try {
    return pkgsource::internal::sha256(data, size);
  } catch (const pkgsource::internal::sha256_error& failure) {
    throw codec_error(
        codec_error_code::invalid_record,
        std::string("failed to compute package-source record checksum: ") +
            failure.what());
  }
}

} // namespace pkgsource::codec::internal
