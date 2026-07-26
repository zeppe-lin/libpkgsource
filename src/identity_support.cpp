// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "identity_support.h"

#include <libpkgsource/error.h>

#include <openssl/evp.h>

#include <array>
#include <iomanip>
#include <memory>
#include <sstream>

namespace pkgsource::detail {
namespace {
using context_ptr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

EVP_MD_CTX* as_context(void* value) noexcept
{
  return static_cast<EVP_MD_CTX*>(value);
}

void update(EVP_MD_CTX* context, const void* data, std::size_t size)
{
  if (EVP_DigestUpdate(context, data, size) != 1)
    throw error(error_code::identity_failed, "SHA-256 update failed");
}

} // namespace

identity_writer::identity_writer() : context_(EVP_MD_CTX_new())
{
  if (!context_ || EVP_DigestInit_ex(as_context(context_), EVP_sha256(), nullptr) != 1) {
    if (context_)
      EVP_MD_CTX_free(as_context(context_));
    context_ = nullptr;
    throw error(error_code::identity_failed, "SHA-256 initialization failed");
  }
}

identity_writer::~identity_writer()
{
  if (context_)
    EVP_MD_CTX_free(as_context(context_));
}

void identity_writer::text(std::string_view value)
{
  number(value.size());
  update(as_context(context_), value.data(), value.size());
}

void identity_writer::number(std::uint64_t value)
{
  std::array<unsigned char, 8> bytes{};
  for (std::size_t i = 0; i < bytes.size(); ++i)
    bytes[bytes.size() - 1 - i] = static_cast<unsigned char>(value >> (i * 8));
  update(as_context(context_), bytes.data(), bytes.size());
}

std::string identity_writer::finish()
{
  if (!context_)
    throw error(error_code::identity_failed, "SHA-256 identity already finalized");
  std::array<unsigned char, EVP_MAX_MD_SIZE> bytes{};
  unsigned int size = 0;
  if (EVP_DigestFinal_ex(as_context(context_), bytes.data(), &size) != 1 || size != 32)
    throw error(error_code::identity_failed, "SHA-256 finalization failed");
  EVP_MD_CTX_free(as_context(context_));
  context_ = nullptr;

  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (unsigned int i = 0; i < size; ++i)
    out << std::setw(2) << static_cast<unsigned int>(bytes[i]);
  return out.str();
}

std::string sha256_hex(std::string_view material)
{
  identity_writer writer;
  writer.text("libpkgsource/content/v1");
  writer.text(material);
  return writer.finish();
}

void require_sha256_hex(std::string_view value)
{
  if (value.size() != 64)
    throw error(error_code::invalid_identity, "SHA-256 value has wrong width");
  for (char c : value) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
      throw error(error_code::invalid_identity,
                  "SHA-256 value is not canonical lowercase hexadecimal");
  }
}

} // namespace pkgsource::detail
