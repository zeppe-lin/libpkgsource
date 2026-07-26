# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

plan_adapter_inc = include_directories('../include')

libpkgsource_plan = library(
  'pkgsource-plan',
  'adapter.cpp',
  include_directories: plan_adapter_inc,
  dependencies: [libpkgsource_dep, libpkgplan_dep, libcrypto_dep],
  install: true,
  version: meson.project_version(),
  soversion: '0',
)

libpkgsource_plan_dep = declare_dependency(
  link_with: libpkgsource_plan,
  include_directories: plan_adapter_inc,
  dependencies: [libpkgsource_dep, libpkgplan_dep],
)

meson.override_dependency('libpkgsource-plan', libpkgsource_plan_dep)

install_headers(
  '../include/libpkgsource-plan/adapter.h',
  subdir: 'libpkgsource-plan',
)

pkgconf.generate(
  libpkgsource_plan,
  name: 'libpkgsource-plan',
  filebase: 'libpkgsource-plan',
  description: 'Explicit libpkgsource to libpkgplan candidate adapter',
  subdirs: '.',
  requires: [
    'libpkgsource = ' + meson.project_version(),
    'libpkgplan >= 0.2.0',
  ],
  requires_private: ['libcrypto'],
)
