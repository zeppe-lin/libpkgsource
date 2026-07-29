# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
public_inc = include_directories('../include')
private_inc = include_directories('.')

libpkgsource = library(
  'pkgsource',
  [
    'error.cpp',
    'identity.cpp',
    'identity_support.cpp',
    'model.cpp',
    'profile.cpp',
    'recipe.cpp',
    'snapshot.cpp',
  ],
  include_directories: [public_inc, private_inc],
  dependencies: libcrypto_dep,
  install: true,
  version: meson.project_version(),
  soversion: '2',
)

libpkgsource_dep = declare_dependency(
  link_with: libpkgsource,
  include_directories: public_inc,
  dependencies: libcrypto_dep,
)
meson.override_dependency('libpkgsource', libpkgsource_dep)

install_headers(
  '../include/libpkgsource/libpkgsource.h',
  '../include/libpkgsource/error.h',
  '../include/libpkgsource/identity.h',
  '../include/libpkgsource/model.h',
  '../include/libpkgsource/profile.h',
  '../include/libpkgsource/recipe.h',
  '../include/libpkgsource/snapshot.h',
  subdir: 'libpkgsource',
)

pkgconf.generate(
  libpkgsource,
  name: 'libpkgsource',
  filebase: 'libpkgsource',
  description: 'Native sealed package-source model library',
  subdirs: '.',
  requires_private: ['libcrypto'],
)
