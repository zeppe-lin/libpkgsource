// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>

namespace pkgsource::detail {

[[nodiscard]] std::string sha256_hex(std::string_view material);
void require_sha256_hex(std::string_view value);

} // namespace pkgsource::detail
