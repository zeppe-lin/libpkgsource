// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pkgsource::internal {

inline constexpr std::size_t sha256_digest_size = 32;
using sha256_digest = std::array<std::uint8_t, sha256_digest_size>;

class sha256_error final : public std::runtime_error {
public:
  explicit sha256_error(std::string message);
};

class sha256_context final {
public:
  sha256_context();
  ~sha256_context();

  sha256_context(const sha256_context&) = delete;
  sha256_context& operator=(const sha256_context&) = delete;
  sha256_context(sha256_context&&) = delete;
  sha256_context& operator=(sha256_context&&) = delete;

  void update(const void* data, std::size_t size);
  void update(std::string_view value);
  [[nodiscard]] sha256_digest finish();

private:
  class state;
  std::unique_ptr<state> state_;
};

[[nodiscard]] sha256_digest sha256(const void* data, std::size_t size);
[[nodiscard]] sha256_digest sha256(std::string_view value);
[[nodiscard]] std::string lowercase_hex(const sha256_digest& digest);

} // namespace pkgsource::internal
