// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "identity_hex.h"

#include <libpkgsource/error.h>

#include "../../internal/sha256.h"

namespace pkgsource::detail {

std::string sha256_hex(std::string_view material)
{
  try {
    return internal::lowercase_hex(internal::sha256(material));
  } catch (const internal::sha256_error& failure) {
    throw error(error_code::identity_failed, failure.what());
  }
}

void require_sha256_hex(std::string_view value)
{
  if (value.size() != 64) {
    throw error(error_code::invalid_identity, "SHA-256 value has wrong width");
  }
  for (const char character : value) {
    const bool decimal = character >= '0' && character <= '9';
    const bool hexadecimal = character >= 'a' && character <= 'f';
    if (!decimal && !hexadecimal) {
      throw error(error_code::invalid_identity,
                  "SHA-256 value is not canonical lowercase hexadecimal");
    }
  }
}

} // namespace pkgsource::detail
