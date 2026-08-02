// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/codec_fixture.h"

namespace test_support::codec_fixture {

void test_nested_and_value_refusals()
{
  const auto catalog = profiles();
  const auto snapshot =
      seal_source(source_origin("recipe.yml"), declaration(true), catalog);

  auto broken_embedded_profile = encode_source_snapshot(snapshot);
  const auto layout = inspect_source_record(broken_embedded_profile);
  assert(layout.profile_blob_size > 42);
  broken_embedded_profile[layout.profile_blob + 10] ^= 0x01U;
  reseal_checksum(broken_embedded_profile);
  expect(codec_error_code::checksum_mismatch, [&] {
    (void)decode_source_snapshot(broken_embedded_profile);
  });

  auto invalid_presence = encode_source_snapshot(snapshot);
  const auto invalid_presence_layout = inspect_source_record(invalid_presence);
  invalid_presence[invalid_presence_layout.description_flag] = 2U;
  reseal_checksum(invalid_presence);
  expect(codec_error_code::invalid_record, [&] {
    (void)decode_source_snapshot(invalid_presence);
  });

  auto invalid_source_kind = encode_source_snapshot(snapshot);
  const auto invalid_kind_layout = inspect_source_record(invalid_source_kind);
  assert(!invalid_kind_layout.sources.empty());
  invalid_source_kind[invalid_kind_layout.sources.front().first] = 9U;
  reseal_checksum(invalid_source_kind);
  expect(codec_error_code::invalid_record, [&] {
    (void)decode_source_snapshot(invalid_source_kind);
  });

  auto excessive_count = encode_profile_catalog(profile_catalog::seal({}));
  set_u32(excessive_count, 10, maximum_record_item_count + 1U);
  reseal_checksum(excessive_count);
  expect(codec_error_code::size_limit, [&] {
    (void)decode_profile_catalog(excessive_count);
  });
}

} // namespace test_support::codec_fixture

int main()
{
  test_support::codec_fixture::test_nested_and_value_refusals();
}
