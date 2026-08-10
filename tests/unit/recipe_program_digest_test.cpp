// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/recipe_fixture.h"

namespace test_support::recipe_fixture {

void test_program_digest_vector()
{
  program value(program_language::posix_shell, "abc");
  assert(value.content_digest().hex() ==
         "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

} // namespace test_support::recipe_fixture

int main()
{
  test_support::recipe_fixture::test_program_digest_vector();
}
