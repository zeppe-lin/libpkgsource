// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/codec_fixture.h"

namespace test_support::codec_fixture {

void test_identity_refusals()
{
  const auto catalog = profiles();
  const auto snapshot =
      seal_source(source_origin("recipe.yml"), declaration(true), catalog);

  auto wrong_source_identity = encode_source_snapshot(snapshot);
  std::size_t offset = 10;
  skip_text(wrong_source_identity, offset);
  const auto identity_size = u32(wrong_source_identity, offset);
  assert(identity_size == 64);
  offset += 4;
  wrong_source_identity[offset] =
      wrong_source_identity[offset] == 'a' ? 'b' : 'a';
  reseal_checksum(wrong_source_identity);
  expect(codec_error_code::identity_mismatch, [&] {
    (void)decode_source_snapshot(wrong_source_identity);
  });

  auto wrong_profile_identity = encode_profile_catalog(catalog);
  offset = 14;
  skip_text(wrong_profile_identity, offset);
  const auto profile_identity_size = u32(wrong_profile_identity, offset);
  assert(profile_identity_size == 64);
  offset += 4;
  wrong_profile_identity[offset] =
      wrong_profile_identity[offset] == 'a' ? 'b' : 'a';
  reseal_checksum(wrong_profile_identity);
  expect(codec_error_code::identity_mismatch, [&] {
    (void)decode_profile_catalog(wrong_profile_identity);
  });
}

} // namespace test_support::codec_fixture

int main()
{
  test_support::codec_fixture::test_identity_refusals();
}
