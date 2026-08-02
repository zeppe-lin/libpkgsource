# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
codec_inc = include_directories('../include')

codec_export_script = configure_file(
  input: '../abi/libpkgsource-codec.exports',
  output: 'libpkgsource-codec.exports.map',
  command: [elf_export_generator, '@INPUT@'],
  capture: true,
)


libpkgsource_codec = library(
  'pkgsource-codec',
  [
    'codec.cpp',
    'internal/profile_catalog_record.cpp',
    'internal/record_checksum.cpp',
    'internal/record_io.cpp',
    'internal/source_snapshot_record.cpp',
    'internal/value_codec.cpp',
  ],
  include_directories: codec_inc,
  dependencies: [libpkgsource_dep, libpkgsource_sha256_provider_dep],
  cpp_args: visibility_cpp_args + ['-DPKGSOURCE_CODEC_BUILDING_LIBRARY'],
  gnu_symbol_visibility: 'hidden',
  link_args: ['-Wl,--version-script=' + codec_export_script.full_path()],
  link_depends: [codec_export_script],
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
  '../include/libpkgsource-codec/libpkgsource-codec.h',
  '../include/libpkgsource-codec/codec.h',
  '../include/libpkgsource-codec/export.h',
  subdir: 'libpkgsource-codec',
)

codec_core_requirement = 'libpkgsource = ' + meson.project_version()

pkgconf.generate(
  libpkgsource_codec,
  name: 'libpkgsource-codec',
  filebase: 'libpkgsource-codec',
  description: 'Canonical durable records for libpkgsource authority',
  subdirs: '.',
  # Meson promotes an in-project dependency by module name but does not carry
  # the version stored in declare_dependency(). Spell the owner requirement
  # explicitly; target-derived metadata for the same module is de-duplicated.
  requires: [codec_core_requirement],
)
