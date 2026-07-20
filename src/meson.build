# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
public_inc = include_directories('../include')
private_inc = include_directories('.')
worker_define = '-DPKGSOURCE_PKGFILE_WORKER="' + pkgfile_worker_install_path + '"'

libpkgsource = library(
  'pkgsource',
  [
    'error.cpp',
    'model.cpp',
    'snapshot_model.cpp',
    'snapshot.cpp',
    'process.cpp',
    'pkgfile_protocol.cpp',
    'pkgfile_backend.cpp',
  ],
  cpp_args: [worker_define],
  include_directories: [public_inc, private_inc],
  dependencies: libcrypto_dep,
  install: true,
  version: meson.project_version(),
  soversion: '0',
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
  '../include/libpkgsource/model.h',
  '../include/libpkgsource/snapshot.h',
  '../include/libpkgsource/backend.h',
  '../include/libpkgsource/pkgfile_backend.h',
  subdir: 'libpkgsource',
)

pkgconf.generate(
  libpkgsource,
  name: 'libpkgsource',
  filebase: 'libpkgsource',
  description: 'Immutable package-source inspection library',
  subdirs: '.',
  requires_private: ['libcrypto'],
)
