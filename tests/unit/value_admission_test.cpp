// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/model_fixture.h"

namespace test_support::model_fixture {

void test_enumeration_admission()
{
  const std::string hash(64, 'a');

  expect(error_code::invalid_identity, [&] {
    (void)digest(static_cast<digest_algorithm>(99), hash);
  });
  assert(to_string(static_cast<digest_algorithm>(99)) == "unknown");

  expect(error_code::invalid_program, [] {
    (void)program(static_cast<program_language>(99), "echo build\n");
  });
  assert(to_string(static_cast<program_language>(99)) == "unknown");

  expect(error_code::invalid_requirement, [] {
    (void)requirement_scope::lifecycle(static_cast<lifecycle_action>(99));
  });
  expect(error_code::invalid_program, [] {
    (void)lifecycle_program(
        static_cast<lifecycle_action>(99),
        program(program_language::posix_shell, "echo lifecycle\n"));
  });
  assert(to_string(static_cast<lifecycle_action>(99)) == "unknown");
}

} // namespace test_support::model_fixture

int main()
{
  test_support::model_fixture::test_enumeration_admission();
}
