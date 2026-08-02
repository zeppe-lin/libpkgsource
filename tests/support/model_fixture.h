// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgsource/libpkgsource.h>

#include <cassert>
#include <functional>
#include <string>
#include <vector>

namespace test_support::model_fixture {

using namespace pkgsource;

template <typename Function> void expect(error_code code, Function&& function)
{
  try {
    function();
    assert(false);
  } catch (const error& value) {
    assert(value.code() == code);
  }
}

} // namespace test_support::model_fixture
