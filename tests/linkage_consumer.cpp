// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/libpkgsource.h>
#include <string>
int main() {
  pkgsource::package_identity id("linkage", "1", "1");
  return id.version_release() == "1-1" ? 0 : 1;
}
