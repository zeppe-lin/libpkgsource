// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

#ifndef TEST_DEPENDENCY_WORKER
#error TEST_DEPENDENCY_WORKER is required
#endif

#ifndef TEST_BUILD_WORKER
#error TEST_BUILD_WORKER is required
#endif

namespace fs = std::filesystem;

namespace {

void require(bool condition, const char* message)
{
  if (!condition)
    throw std::runtime_error(message);
}

} // namespace

int main()
{
  try {
    const fs::path dependency =
        fs::path(TEST_DEPENDENCY_WORKER).lexically_normal();
    const fs::path build = fs::path(TEST_BUILD_WORKER).lexically_normal();

    require(dependency.is_absolute(),
            "dependency worker path is not absolute");
    require(dependency == build,
            "internal dependency worker path differs from build worker");
    require(fs::is_regular_file(dependency),
            "dependency worker is not a regular file");
    require(::access(dependency.c_str(), X_OK) == 0,
            "dependency worker is not executable");
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "worker-dependency-test: " << error.what() << '\n';
    return 1;
  }
}
