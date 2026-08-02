// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../internal/sha256.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pkgsource::detail {

class identity_writer final {
public:
  void text(std::string_view value);
  void number(std::uint64_t value);
  [[nodiscard]] std::string finish();

private:
  internal::sha256_context context_;
};

} // namespace pkgsource::detail
