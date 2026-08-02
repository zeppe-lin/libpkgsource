// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgsource-codec/codec.h>
#include <libpkgsource/error.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pkgsource::codec::internal {

inline constexpr std::size_t record_checksum_size = 32;

[[noreturn]] void fail(codec_error_code code, std::string message);
[[noreturn]] void translate_model_failure(const error& failure);

class record_writer final {
public:
  explicit record_writer(std::size_t maximum);

  void u8(std::uint8_t value);
  void u16(std::uint16_t value);
  void u32(std::uint32_t value);
  void u64(std::uint64_t value);

  template <std::size_t Size>
  void raw(const std::array<std::uint8_t, Size>& value)
  {
    bytes_.insert(bytes_.end(), value.begin(), value.end());
    check_size();
  }

  void raw(const std::vector<std::uint8_t>& value);
  void text(std::string_view value);
  void blob(const std::vector<std::uint8_t>& value);
  [[nodiscard]] std::vector<std::uint8_t> finish();

private:
  void check_size() const;

  std::size_t maximum_;
  std::vector<std::uint8_t> bytes_;
};

class record_reader final {
public:
  record_reader(const std::vector<std::uint8_t>& bytes, std::size_t maximum);

  [[nodiscard]] std::uint8_t u8();
  [[nodiscard]] std::uint16_t u16();
  [[nodiscard]] std::uint32_t u32();
  [[nodiscard]] std::uint64_t u64();

  template <std::size_t Size>
  void expect(const std::array<std::uint8_t, Size>& expected,
              std::string_view name)
  {
    require(Size);
    if (!std::equal(
            expected.begin(), expected.end(), bytes_.begin() + offset_)) {
      fail(codec_error_code::invalid_magic,
           "invalid " + std::string(name) + " magic");
    }
    offset_ += Size;
  }

  [[nodiscard]] std::string text();
  [[nodiscard]] std::vector<std::uint8_t> blob(std::size_t maximum);
  void finish() const;

private:
  void require(std::size_t size) const;

  const std::vector<std::uint8_t>& bytes_;
  std::size_t limit_;
  std::size_t offset_ = 0;
};

[[nodiscard]] std::uint32_t record_count(std::size_t value);
[[nodiscard]] std::uint32_t read_record_count(record_reader& input);

} // namespace pkgsource::codec::internal
