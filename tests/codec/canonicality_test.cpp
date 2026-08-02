// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/codec_fixture.h"

namespace test_support::codec_fixture {

void test_noncanonical_source_order()
{
  const auto catalog = profiles();
  const auto snapshot =
      seal_source(source_origin("recipe.yml"), declaration(false), catalog);
  const auto canonical = encode_source_snapshot(snapshot);
  auto reordered = canonical;
  const auto layout = inspect_source_record(reordered);
  assert(u32(reordered, layout.source_count) == 2U);
  assert(layout.sources.size() == 2U);

  const auto first = std::vector<std::uint8_t>(
      reordered.begin() + static_cast<std::ptrdiff_t>(layout.sources[0].first),
      reordered.begin() +
          static_cast<std::ptrdiff_t>(layout.sources[0].second));
  const auto second = std::vector<std::uint8_t>(
      reordered.begin() + static_cast<std::ptrdiff_t>(layout.sources[1].first),
      reordered.begin() +
          static_cast<std::ptrdiff_t>(layout.sources[1].second));
  auto output =
      reordered.begin() + static_cast<std::ptrdiff_t>(layout.sources[0].first);
  output = std::copy(second.begin(), second.end(), output);
  std::copy(first.begin(), first.end(), output);
  reseal_checksum(reordered);
  assert(reordered != canonical);

  expect(codec_error_code::noncanonical, [&] {
    (void)decode_source_snapshot(reordered);
  });
}

} // namespace test_support::codec_fixture

int main()
{
  test_support::codec_fixture::test_noncanonical_source_order();
}
