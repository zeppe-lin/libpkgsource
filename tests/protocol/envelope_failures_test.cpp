// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/codec_fixture.h"

namespace test_support::codec_fixture {

void test_envelope_refusals()
{
  const auto catalog = profiles();
  const auto snapshot =
      seal_source(source_origin("recipe.yml"), declaration(true), catalog);

  auto corrupt = encode_source_snapshot(snapshot);
  corrupt[20] ^= 0x01U;
  expect(codec_error_code::checksum_mismatch, [&] {
    (void)decode_source_snapshot(corrupt);
  });

  auto truncated = encode_source_snapshot(snapshot);
  truncated.resize(17);
  expect(codec_error_code::truncated, [&] {
    (void)decode_source_snapshot(truncated);
  });

  auto bad_magic = encode_source_snapshot(snapshot);
  bad_magic[0] = 'X';
  reseal_checksum(bad_magic);
  expect(codec_error_code::invalid_magic, [&] {
    (void)decode_source_snapshot(bad_magic);
  });

  auto bad_version = encode_source_snapshot(snapshot);
  bad_version[8] = 0;
  bad_version[9] = 2;
  reseal_checksum(bad_version);
  expect(codec_error_code::unsupported_version, [&] {
    (void)decode_source_snapshot(bad_version);
  });

  auto trailing = encode_source_snapshot(snapshot);
  trailing.insert(trailing.end() - 32, 0U);
  reseal_checksum(trailing);
  expect(codec_error_code::invalid_record, [&] {
    (void)decode_source_snapshot(trailing);
  });

  std::vector<std::uint8_t> oversized(
      maximum_profile_catalog_encoding_size + 1U, 0U);
  expect(codec_error_code::size_limit, [&] {
    (void)decode_profile_catalog(oversized);
  });
}

} // namespace test_support::codec_fixture

int main()
{
  test_support::codec_fixture::test_envelope_refusals();
}
