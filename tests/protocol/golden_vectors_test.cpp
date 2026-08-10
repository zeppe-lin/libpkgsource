// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/codec_fixture.h"

namespace test_support::codec_fixture {

void test_golden_vectors()
{
  const auto empty_profiles = profile_catalog::seal({});
  const auto profile_encoding = encode_profile_catalog(empty_profiles);
  assert(profile_encoding.size() == 46);
  assert(sha256_hex(profile_encoding) ==
         "2f268947090f17e2c4f1825c0c7167930c8950327c6389c6531d8e6f64b4e483");

  const auto snapshot = seal_source(
      source_origin("recipe.yml"), minimal_declaration(), empty_profiles);
  assert(snapshot.identity().hex() ==
         "485b072c47308c1c64e8ec9d8c88c2418c57642c2b50e4e59c853c509d5da838");
  const auto source_encoding = encode_source_snapshot(snapshot);
  assert(source_encoding.size() == 275);
  assert(sha256_hex(source_encoding) ==
         "cd221e9527162de41fa23806f2a370e161139cf059c6dc77d08cbfd37b45be35");
}

} // namespace test_support::codec_fixture

int main()
{
  test_support::codec_fixture::test_golden_vectors();
}
