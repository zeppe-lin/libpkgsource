// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sha256.h"

#include <openssl/evp.h>

#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>

namespace pkgsource::internal {
namespace {

using context_pointer = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

[[nodiscard]] context_pointer make_context()
{
  context_pointer context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!context ||
      EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1) {
    throw sha256_error("OpenSSL SHA-256 initialization failed");
  }
  return context;
}

} // namespace

class sha256_context::state final {
public:
  state() : context(make_context())
  {
  }

  context_pointer context;
  bool finished = false;
};

sha256_error::sha256_error(std::string message)
    : std::runtime_error(std::move(message))
{
}

sha256_context::sha256_context() : state_(std::make_unique<state>())
{
}

sha256_context::~sha256_context() = default;

void sha256_context::update(const void* data, std::size_t size)
{
  if (state_->finished) {
    throw sha256_error("SHA-256 operation is already finalized");
  }
  if (size == 0) {
    return;
  }
  if (data == nullptr) {
    throw sha256_error("SHA-256 input pointer is null for non-empty input");
  }
  if (EVP_DigestUpdate(state_->context.get(), data, size) != 1) {
    throw sha256_error("OpenSSL SHA-256 update failed");
  }
}

void sha256_context::update(std::string_view value)
{
  update(value.data(), value.size());
}

sha256_digest sha256_context::finish()
{
  if (state_->finished) {
    throw sha256_error("SHA-256 operation is already finalized");
  }

  sha256_digest digest{};
  unsigned int size = 0;
  if (EVP_DigestFinal_ex(state_->context.get(), digest.data(), &size) != 1 ||
      size != digest.size()) {
    throw sha256_error("OpenSSL SHA-256 finalization failed");
  }

  state_->finished = true;
  return digest;
}

sha256_digest sha256(const void* data, std::size_t size)
{
  sha256_context context;
  context.update(data, size);
  return context.finish();
}

sha256_digest sha256(std::string_view value)
{
  return sha256(value.data(), value.size());
}

std::string lowercase_hex(const sha256_digest& digest)
{
  std::ostringstream output;
  output << std::hex << std::setfill('0');
  for (const std::uint8_t byte : digest) {
    output << std::setw(2) << static_cast<unsigned int>(byte);
  }
  return output.str();
}

} // namespace pkgsource::internal
