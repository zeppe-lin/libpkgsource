// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pkgsource::detail {

class identity_writer final {
public:
  identity_writer();
  ~identity_writer();
  identity_writer(const identity_writer&) = delete;
  identity_writer& operator=(const identity_writer&) = delete;
  void text(std::string_view value);
  void number(std::uint64_t value);
  [[nodiscard]] std::string finish();
private:
  void* context_;
};

[[nodiscard]] std::string sha256_hex(std::string_view material);
void require_sha256_hex(std::string_view value);

} // namespace pkgsource::detail
