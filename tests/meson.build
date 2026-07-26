# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  corpus_path = join_paths(meson.current_source_dir(), 'corpus')
  test_cpp_args = [
    '-DTEST_WORKER="' + pkgfile_worker_build_path + '"',
    '-DTEST_CORPUS="' + corpus_path + '"',
  ]

  unit_tests = executable(
    'libpkgsource-tests',
    'test_main.cpp',
    cpp_args: test_cpp_args,
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('unit', unit_tests)

  public_headers = executable(
    'public-headers',
    'public_headers.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('public-headers', public_headers)

  if libpkgplan_dep.found()
    plan_adapter_test = executable(
      'plan-adapter-test',
      'plan_adapter_test.cpp',
      cpp_args: test_cpp_args,
      dependencies: libpkgsource_plan_dep,
      install: false,
    )
    test('plan-adapter', plan_adapter_test)

    plan_adapter_header = executable(
      'plan-adapter-header-test',
      'plan_adapter_header_test.cpp',
      dependencies: libpkgsource_plan_dep,
      install: false,
    )
    test('plan-adapter-header', plan_adapter_header)
  endif


  linkage_consumer = executable(
    'linkage-consumer',
    'linkage_consumer.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
  test('linkage-current-configuration', linkage_consumer)


  release_metadata = find_program('check_release_metadata.sh')
  test(
    'release-metadata',
    release_metadata,
    args: [meson.project_source_root()],
  )

  ci_contract = find_program('check_ci_contract.sh')
  test(
    'ci-contract',
    ci_contract,
    args: [meson.project_source_root()],
  )

  blackbox = find_program(join_paths(meson.current_source_dir(), 'blackbox.sh'))
  test(
    'reference-tool-black-box',
    blackbox,
    args: [
      pkgsource_inspect,
      pkgfile_worker,
      join_paths(corpus_path, 'complete', 'demo'),
    ],
  )
endif
