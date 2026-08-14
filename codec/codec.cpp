// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource-codec/codec.h>

#include <utility>

namespace pkgsource::codec {

codec_error::codec_error(codec_error_code code, std::string message)
    : std::invalid_argument(std::move(message)), code_(code)
{
}

codec_error::~codec_error() = default;

codec_error_code codec_error::code() const noexcept
{
  return code_;
}

} // namespace pkgsource::codec
