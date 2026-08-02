// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "record_io.h"

#include "record_checksum.h"

#include <limits>
#include <utility>

namespace pkgsource::codec::internal {

[[noreturn]] void fail(codec_error_code code, std::string message)
{
  throw codec_error(code, std::move(message));
}

[[noreturn]] void translate_model_failure(const error& failure)
{
  fail(codec_error_code::invalid_record,
       std::string("invalid package-source record: ") + failure.what());
}

record_writer::record_writer(std::size_t maximum) : maximum_(maximum)
{
}

void record_writer::u8(std::uint8_t value)
{
  bytes_.push_back(value);
  check_size();
}

void record_writer::u16(std::uint16_t value)
{
  u8(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
  u8(static_cast<std::uint8_t>(value & 0xffU));
}

void record_writer::u32(std::uint32_t value)
{
  for (int shift = 24; shift >= 0; shift -= 8) {
    u8(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }
}

void record_writer::u64(std::uint64_t value)
{
  for (int shift = 56; shift >= 0; shift -= 8) {
    u8(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }
}

void record_writer::raw(const std::vector<std::uint8_t>& value)
{
  bytes_.insert(bytes_.end(), value.begin(), value.end());
  check_size();
}

void record_writer::text(std::string_view value)
{
  if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
    fail(codec_error_code::size_limit,
         "package-source record text exceeds encoding limit");
  }
  u32(static_cast<std::uint32_t>(value.size()));
  bytes_.insert(bytes_.end(), value.begin(), value.end());
  check_size();
}

void record_writer::blob(const std::vector<std::uint8_t>& value)
{
  u64(static_cast<std::uint64_t>(value.size()));
  raw(value);
}

std::vector<std::uint8_t> record_writer::finish()
{
  const auto digest = record_checksum(bytes_.data(), bytes_.size());
  raw(digest);
  return std::move(bytes_);
}

void record_writer::check_size() const
{
  if (bytes_.size() > maximum_ - record_checksum_size) {
    fail(codec_error_code::size_limit,
         "package-source record exceeds encoding limit");
  }
}

record_reader::record_reader(const std::vector<std::uint8_t>& bytes,
                             std::size_t maximum)
    : bytes_(bytes), limit_(bytes.size() >= record_checksum_size
                                ? bytes.size() - record_checksum_size
                                : 0U)
{
  if (bytes.size() > maximum) {
    fail(codec_error_code::size_limit,
         "package-source record exceeds decoding limit");
  }
  if (bytes.size() < record_checksum_size) {
    fail(codec_error_code::truncated,
         "package-source record is shorter than its checksum");
  }
  const auto actual = record_checksum(bytes.data(), limit_);
  if (!std::equal(actual.begin(), actual.end(), bytes.begin() + limit_)) {
    fail(codec_error_code::checksum_mismatch,
         "package-source record checksum mismatch");
  }
}

std::uint8_t record_reader::u8()
{
  require(1U);
  return bytes_[offset_++];
}

std::uint16_t record_reader::u16()
{
  return static_cast<std::uint16_t>(u8()) << 8U |
         static_cast<std::uint16_t>(u8());
}

std::uint32_t record_reader::u32()
{
  std::uint32_t value = 0;
  for (int index = 0; index != 4; ++index) {
    value = (value << 8U) | u8();
  }
  return value;
}

std::uint64_t record_reader::u64()
{
  std::uint64_t value = 0;
  for (int index = 0; index != 8; ++index) {
    value = (value << 8U) | u8();
  }
  return value;
}

std::string record_reader::text()
{
  const auto size = u32();
  require(size);
  std::string result(reinterpret_cast<const char*>(bytes_.data() + offset_),
                     size);
  offset_ += size;
  return result;
}

std::vector<std::uint8_t> record_reader::blob(std::size_t maximum)
{
  const auto size = u64();
  if (size > maximum || size > std::numeric_limits<std::size_t>::max()) {
    fail(codec_error_code::size_limit,
         "embedded package-source record exceeds decoding limit");
  }
  require(static_cast<std::size_t>(size));
  std::vector<std::uint8_t> result(
      bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
      bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
  offset_ += static_cast<std::size_t>(size);
  return result;
}

void record_reader::finish() const
{
  if (offset_ != limit_) {
    fail(codec_error_code::invalid_record,
         "package-source record has trailing fields");
  }
}

void record_reader::require(std::size_t size) const
{
  if (offset_ > limit_ || size > limit_ - offset_) {
    fail(codec_error_code::truncated, "package-source record is truncated");
  }
}

std::uint32_t record_count(std::size_t value)
{
  if (value > maximum_record_item_count) {
    fail(codec_error_code::size_limit,
         "package-source collection exceeds item limit");
  }
  return static_cast<std::uint32_t>(value);
}

std::uint32_t read_record_count(record_reader& input)
{
  const auto value = input.u32();
  if (value > maximum_record_item_count) {
    fail(codec_error_code::size_limit,
         "package-source collection exceeds item limit");
  }
  return value;
}

} // namespace pkgsource::codec::internal
