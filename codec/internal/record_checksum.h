// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../../internal/sha256.h"

#include <cstddef>
#include <cstdint>

namespace pkgsource::codec::internal {

[[nodiscard]] pkgsource::internal::sha256_digest
record_checksum(const std::uint8_t* data, std::size_t size);

} // namespace pkgsource::codec::internal
