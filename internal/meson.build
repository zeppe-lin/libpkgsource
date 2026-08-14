# SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

repository_internal_inc = include_directories('..')

sha256_provider = get_option('sha256_provider')
sha256_provider_sources = []
sha256_provider_dependencies = []

if sha256_provider == 'openssl'
  sha256_provider_sources += files('sha256_openssl.cpp')
  sha256_provider_dependencies += [libcrypto_dep]
else
  error('unsupported SHA-256 provider: ' + sha256_provider)
endif

libpkgsource_sha256_provider = static_library(
  'pkgsource-sha256-provider',
  sha256_provider_sources,
  include_directories: repository_internal_inc,
  dependencies: sha256_provider_dependencies,
  gnu_symbol_visibility: 'hidden',
  install: false,
  pic: true,
)

libpkgsource_sha256_provider_dep = declare_dependency(
  link_with: libpkgsource_sha256_provider,
  include_directories: repository_internal_inc,
  dependencies: sha256_provider_dependencies,
)
