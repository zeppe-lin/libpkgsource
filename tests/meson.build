# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  model_test = executable(
    'model-test',
    'model/value_domains_test.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('model', model_test)

  profile_test = executable(
    'profile-test',
    'profiles/sealing_test.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('profile', profile_test)

  recipe_test = executable(
    'recipe-test',
    'recipes/sealing_test.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('recipe', recipe_test)

  codec_test = executable(
    'codec-test',
    'codec/codec_test.cpp',
    dependencies: libpkgsource_codec_dep,
    install: false,
  )
  test('codec', codec_test)

  public_headers = executable(
    'public-headers',
    'public/core_umbrella_header_test.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('public-headers', public_headers)

  codec_public_headers = executable(
    'codec-public-headers',
    'public/codec_header_test.cpp',
    dependencies: libpkgsource_codec_dep,
    install: false,
  )
  test('codec-public-headers', codec_public_headers)

  codec_contract = find_program('contracts/check_codec_contract.sh')
  test(
    'codec-contract',
    codec_contract,
    args: [meson.project_source_root()],
  )


  core_boundary = find_program('contracts/check_core_boundary.sh')
  test(
    'core-boundary',
    core_boundary,
    args: [meson.project_source_root()],
  )

  documentation_contract = find_program('contracts/check_documentation_contract.sh')
  test(
    'documentation-contract',
    documentation_contract,
    args: [meson.project_source_root()],
  )

  pkgconfig_metadata = find_program('contracts/pkgconfig_metadata_test.sh')
  test(
    'pkgconfig-metadata',
    pkgconfig_metadata,
    args: [meson.project_build_root(), meson.project_version()],
  )

  release_metadata = find_program('contracts/check_release_metadata.sh')
  test(
    'release-metadata',
    release_metadata,
    args: [meson.project_source_root()],
  )
endif
