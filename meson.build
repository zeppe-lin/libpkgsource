# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
project(
  'libpkgsource',
  'cpp',
  version: '0.1.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>=1.2.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'default_library=shared',
    'b_ndebug=if-release',
  ],
)

library_kind = get_option('default_library')
if library_kind == 'both'
  error('libpkgsource requires separate shared and static builds')
endif

want_static = get_option('link_mode') == 'static'
if (library_kind == 'static') != want_static
  error('default_library and link_mode must both select shared or static')
endif

libcrypto_dep = dependency('libcrypto', required: true, static: want_static)
pkgconf = import('pkgconfig')

pkgfile_worker_install_path = join_paths(
  get_option('prefix'),
  get_option('libexecdir'),
  'pkgsource-pkgfile-worker',
)

pkgfile_worker = configure_file(
  input: 'src/pkgsource-pkgfile-worker.in',
  output: 'pkgsource-pkgfile-worker',
  copy: true,
  install: true,
  install_dir: get_option('libexecdir'),
  install_mode: 'rwxr-xr-x',
)
pkgfile_worker_build_path = join_paths(
  meson.project_build_root(),
  'pkgsource-pkgfile-worker',
)

subdir('src')
subdir('tools')
subdir('tests')
