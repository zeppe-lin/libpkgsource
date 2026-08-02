# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
codec_inc = include_directories('../include')

libpkgsource_codec = library(
  'pkgsource-codec',
  [
    'codec.cpp',
    'internal/record_checksum.cpp',
  ],
  include_directories: codec_inc,
  dependencies: [libpkgsource_dep, libpkgsource_sha256_provider_dep],
  install: true,
  version: meson.project_version(),
  soversion: '1',
)

libpkgsource_codec_dep = declare_dependency(
  link_with: libpkgsource_codec,
  include_directories: codec_inc,
  dependencies: [libpkgsource_dep, libpkgsource_sha256_provider_dep],
)
meson.override_dependency('libpkgsource-codec', libpkgsource_codec_dep)

install_headers(
  '../include/libpkgsource-codec/codec.h',
  subdir: 'libpkgsource-codec',
)

pkgconf.generate(
  libpkgsource_codec,
  name: 'libpkgsource-codec',
  filebase: 'libpkgsource-codec',
  description: 'Canonical durable records for libpkgsource authority',
  subdirs: '.',
  requires: ['libpkgsource = ' + meson.project_version()],
  requires_private: ['libcrypto'],
)
