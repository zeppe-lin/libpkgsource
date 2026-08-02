# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  model_test = executable(
    'model-test',
    'model_test.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('model', model_test)

  profile_test = executable(
    'profile-test',
    'profile_test.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('profile', profile_test)

  recipe_test = executable(
    'recipe-test',
    'recipe_test.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('recipe', recipe_test)

  codec_test = executable(
    'codec-test',
    'codec_test.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('codec', codec_test)

  public_headers = executable(
    'public-headers',
    'public_headers.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('public-headers', public_headers)

  codec_contract = find_program('check_codec_contract.sh')
  test(
    'codec-contract',
    codec_contract,
    args: [meson.project_source_root()],
  )

  release_metadata = find_program('check_release_metadata.sh')
  test(
    'release-metadata',
    release_metadata,
    args: [meson.project_source_root()],
  )
endif
