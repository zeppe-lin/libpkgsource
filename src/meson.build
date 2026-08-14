# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
public_inc = include_directories('../include')
private_inc = include_directories('.')

elf_export_generator = find_program(
  meson.project_source_root() / 'tools/generate-elf-export-script.sh',
)
core_export_script_name = 'libpkgsource.exports.map'
core_export_script_path = meson.current_build_dir() / core_export_script_name
core_export_script = configure_file(
  input: '../abi/libpkgsource.exports',
  output: core_export_script_name,
  command: [elf_export_generator, '@INPUT@'],
  capture: true,
)


libpkgsource = library(
  'pkgsource',
  [
    'error.cpp',
    'identity.cpp',
    'internal/identity_hex.cpp',
    'internal/identity_writer.cpp',
    'model.cpp',
    'profile.cpp',
    'recipe.cpp',
    'snapshot.cpp',
  ],
  include_directories: [public_inc, private_inc],
  dependencies: libpkgsource_sha256_provider_dep,
  cpp_args: visibility_cpp_args + ['-DPKGSOURCE_BUILDING_LIBRARY'],
  gnu_symbol_visibility: 'hidden',
  link_args: ['-Wl,--version-script=' + core_export_script_path],
  link_depends: [core_export_script],
  install: true,
  version: meson.project_version(),
  soversion: '4',
)

# The version is part of the codec's generated public pkg-config requirement.
# Meson promotes this same dependency object from the target's private closure.
libpkgsource_dep = declare_dependency(
  link_with: libpkgsource,
  include_directories: public_inc,
  dependencies: libpkgsource_sha256_provider_dep,
  version: meson.project_version(),
)
meson.override_dependency('libpkgsource', libpkgsource_dep)

install_headers(
  '../include/libpkgsource/libpkgsource.h',
  '../include/libpkgsource/error.h',
  '../include/libpkgsource/export.h',
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
)
