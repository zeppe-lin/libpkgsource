// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../internal/sha256.h"

#include <cassert>
#include <string>

int main()
{
  using pkgsource::internal::lowercase_hex;
  using pkgsource::internal::sha256;
  using pkgsource::internal::sha256_context;
  using pkgsource::internal::sha256_error;

  assert(lowercase_hex(sha256("")) ==
         "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  assert(lowercase_hex(sha256("abc")) ==
         "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

  sha256_context streamed;
  streamed.update("a");
  streamed.update("bc");
  assert(lowercase_hex(streamed.finish()) ==
         "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

  bool repeated_finish_rejected = false;
  try {
    (void)streamed.finish();
  } catch (const sha256_error&) {
    repeated_finish_rejected = true;
  }
  assert(repeated_finish_rejected);

  sha256_context null_input;
  bool null_rejected = false;
  try {
    null_input.update(nullptr, 1);
  } catch (const sha256_error&) {
    null_rejected = true;
  }
  assert(null_rejected);
}
