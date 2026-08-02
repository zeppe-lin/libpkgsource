// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "identity_writer.h"

#include <libpkgsource/error.h>

#include <array>

namespace pkgsource::detail {
namespace {

[[noreturn]] void
translate_provider_failure(const internal::sha256_error& failure)
{
  throw error(error_code::identity_failed, failure.what());
}

} // namespace

void identity_writer::text(std::string_view value)
{
  number(value.size());
  try {
    context_.update(value);
  } catch (const internal::sha256_error& failure) {
    translate_provider_failure(failure);
  }
}

void identity_writer::number(std::uint64_t value)
{
  std::array<std::uint8_t, 8> bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    bytes[bytes.size() - 1 - index] =
        static_cast<std::uint8_t>(value >> (index * 8));
  }
  try {
    context_.update(bytes.data(), bytes.size());
  } catch (const internal::sha256_error& failure) {
    translate_provider_failure(failure);
  }
}

std::string identity_writer::finish()
{
  try {
    return internal::lowercase_hex(context_.finish());
  } catch (const internal::sha256_error& failure) {
    translate_provider_failure(failure);
  }
}

} // namespace pkgsource::detail
