// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/codec_fixture.h"

namespace test_support::codec_fixture {

void test_source_snapshot_round_trip()
{
  const auto catalog = profiles();
  const auto plain = seal_source(
      source_origin("recipes/example/recipe.yml"), declaration(false), catalog);
  const auto plain_encoding = encode_source_snapshot(plain);
  const auto plain_decoded = decode_source_snapshot(plain_encoding);
  assert(plain_decoded.identity() == plain.identity());
  assert(plain_decoded.origin().document() == plain.origin().document());
  assert(plain_decoded.recipe().build_requirements().size() == 2);
  assert(plain_decoded.recipe().run_requirements().size() == 1);
  assert(plain_decoded.recipe().profile_closure().size() == 3);
  assert(encode_source_snapshot(plain_decoded) == plain_encoding);

  const auto checked = seal_source(
      source_origin("recipes/example/recipe.yml"), declaration(true), catalog);
  const auto checked_encoding = encode_source_snapshot(checked);
  const auto checked_decoded = decode_source_snapshot(checked_encoding);
  assert(checked_decoded.identity() == checked.identity());
  assert(checked_decoded.recipe().check_program());
  assert(checked_decoded.recipe().check_program()->material() ==
         "meson test -C build\n");
  assert(encode_source_snapshot(checked_decoded) == checked_encoding);
}

} // namespace test_support::codec_fixture

int main()
{
  test_support::codec_fixture::test_source_snapshot_round_trip();
}
