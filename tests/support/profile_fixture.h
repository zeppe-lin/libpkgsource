// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgsource/libpkgsource.h>

#include <cassert>
#include <functional>
#include <vector>

namespace test_support::profile_fixture {

using namespace pkgsource;

declaration_provenance at(const char* path, std::uint32_t line)
{
  return declaration_provenance("profiles.yml", path, line, 3);
}

template <typename Function> void expect(error_code code, Function&& function)
{
  try {
    function();
    assert(false);
  } catch (const error& value) {
    assert(value.code() == code);
  }
}

profile_declaration
leaf(const char* name, const char* package, std::uint32_t line)
{
  return profile_declaration(
      profile_reference(name),
      at(name, line),
      {profile_member_declaration(
          requirement_subject(package_reference(package)),
          at(package, line + 1))});
}

} // namespace test_support::profile_fixture
