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

  if libpkgplan_dep.found()
    plan_adapter_test = executable(
      'plan-adapter-test',
      'plan_adapter_test.cpp',
      dependencies: libpkgsource_plan_dep,
      install: false,
    )
    test('plan-adapter', plan_adapter_test)

    plan_adapter_header_test = executable(
      'plan-adapter-header-test',
      'plan_adapter_header_test.cpp',
      dependencies: libpkgsource_plan_dep,
      install: false,
    )
    test('plan-adapter-header', plan_adapter_header_test)
  endif

  public_headers = executable(
    'public-headers',
    'public_headers.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('public-headers', public_headers)
endif
