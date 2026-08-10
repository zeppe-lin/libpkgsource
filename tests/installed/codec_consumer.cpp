// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource-codec/libpkgsource-codec.h>

int main()
{
  const pkgsource::profile_catalog catalog = pkgsource::profile_catalog::seal({});
  const auto encoding = pkgsource::codec::encode_profile_catalog(catalog);
  const auto decoded = pkgsource::codec::decode_profile_catalog(encoding);
  return decoded.profiles().empty() ? 0 : 1;
}
