# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  unit_tests = {
    'model-identity': 'unit/model_identity_test.cpp',
    'model-requirements': 'unit/model_requirements_test.cpp',
    'model-values': 'unit/model_values_test.cpp',
    'value-admission': 'unit/value_admission_test.cpp',
    'profile-expansion': 'unit/profile_expansion_test.cpp',
    'profile-requirements': 'unit/profile_requirements_test.cpp',
    'profile-identity': 'unit/profile_identity_test.cpp',
    'profile-duplicates': 'unit/profile_duplicate_test.cpp',
    'profile-failures': 'unit/profile_failures_test.cpp',
    'sealed-profile': 'unit/sealed_profile_test.cpp',
    'recipe-content': 'unit/recipe_content_test.cpp',
    'recipe-normalization': 'unit/recipe_normalization_test.cpp',
    'recipe-check-program': 'unit/recipe_check_program_test.cpp',
    'recipe-program-digest': 'unit/recipe_program_digest_test.cpp',
    'recipe-failures': 'unit/recipe_failures_test.cpp',
    'sealed-recipe': 'unit/sealed_recipe_test.cpp',
  }
  foreach test_name, source_file : unit_tests
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_dep,
      install: false,
    )
    test(test_name, executable_target, suite: 'unit')
  endforeach

  integration_tests = {
    'source-identity': 'integration/source_identity_test.cpp',
    'source-authority': 'integration/source_authority_test.cpp',
  }
  foreach test_name, source_file : integration_tests
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_dep,
      install: false,
    )
    test(test_name, executable_target, suite: 'integration')
  endforeach

  protocol_tests = {
    'codec-golden-vectors': 'protocol/golden_vectors_test.cpp',
    'codec-profile-catalog': 'protocol/profile_catalog_test.cpp',
    'codec-source-snapshot': 'protocol/source_snapshot_test.cpp',
    'codec-envelope-failures': 'protocol/envelope_failures_test.cpp',
    'codec-identity-failures': 'protocol/identity_failures_test.cpp',
    'codec-value-failures': 'protocol/value_failures_test.cpp',
    'codec-canonicality': 'protocol/canonicality_test.cpp',
  }
  foreach test_name, source_file : protocol_tests
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_codec_dep,
      install: false,
    )
    test(test_name, executable_target, suite: 'protocol')
  endforeach

  sha256_provider_test = executable(
    'sha256-provider-test',
    [
      'mechanism/sha256_test.cpp',
      '../internal/sha256_openssl.cpp',
    ],
    include_directories: repository_internal_inc,
    dependencies: libcrypto_dep,
    install: false,
  )
  test('sha256-provider', sha256_provider_test, suite: 'mechanism')

  core_public_headers = {
    'public-core-umbrella': 'header/core_umbrella_header_test.cpp',
    'public-error-header': 'header/error_header_test.cpp',
    'public-identity-header': 'header/identity_header_test.cpp',
    'public-model-header': 'header/model_header_test.cpp',
    'public-profile-header': 'header/profile_header_test.cpp',
    'public-recipe-header': 'header/recipe_header_test.cpp',
    'public-snapshot-header': 'header/snapshot_header_test.cpp',
  }
  foreach test_name, source_file : core_public_headers
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_dep,
      install: false,
    )
    test(test_name, executable_target, suite: 'header')
  endforeach

  codec_public_headers = {
    'public-codec-component': 'header/codec_header_test.cpp',
    'public-codec-umbrella': 'header/codec_umbrella_header_test.cpp',
  }
  foreach test_name, source_file : codec_public_headers
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_codec_dep,
      install: false,
    )
    test(test_name, executable_target, suite: 'header')
  endforeach

  sh = find_program('sh')
  if library_kind == 'shared'
    nm = find_program('nm')
    foreach abi_spec : [
      ['core-abi-surface', libpkgsource, meson.project_source_root() / 'abi/libpkgsource.exports'],
      ['codec-abi-surface', libpkgsource_codec, meson.project_source_root() / 'abi/libpkgsource-codec.exports'],
    ]
      test(
        abi_spec[0],
        sh,
        args: [
          meson.current_source_dir() / 'contracts/check_abi_surface.sh',
          nm.full_path(),
          abi_spec[1].full_path(),
          abi_spec[2],
        ],
        suite: 'contracts',
      )
    endforeach
  endif

  foreach contract : [
    'codec-contract',
    'core-boundary',
    'documentation-contract',
    'documentation-install-contract',
    'doxygen-contract',
    'html-docs-contract',
    'provider-boundary',
    'release-metadata',
    'style-contract',
    'test-structure',
    'manpage-source',
    'manpage-normalizer',
  ]
    test(
      contract,
      sh,
      args: [
        meson.current_source_dir() / ('contracts/check_' + contract.replace('-', '_') + '.sh'),
        meson.project_source_root(),
      ] + (contract == 'style-contract' ? [meson.project_build_root()] : []),
      suite: 'contracts',
    )
  endforeach

  pkgconfig_metadata = find_program('contracts/pkgconfig_metadata_test.sh')
  test(
    'pkgconfig-metadata',
    pkgconfig_metadata,
    args: [meson.project_build_root(), meson.project_version()],
    suite: 'contracts',
  )

  pandoc = find_program('pandoc', required: false)
  if pandoc.found()
    test(
      'manpage-generated',
      sh,
      args: [
        meson.current_source_dir() / 'contracts/check_manpage_generated.sh',
        meson.project_source_root(),
        pandoc.full_path(),
      ],
      suite: 'contracts',
    )
  endif

  clang_format = find_program('clang-format-17', required: false)
  if clang_format.found()
    test(
      'format',
      sh,
      args: [
        meson.current_source_dir() / 'contracts/check_format.sh',
        clang_format.full_path(),
        meson.project_source_root(),
      ],
      suite: 'contracts',
    )
  endif

  if html_docs_enabled
    html_docs_checker = find_program(
      meson.project_source_root() / 'tools/check-html-docs.py',
    )
    test(
      'html-docs',
      html_docs_checker,
      args: [
        html_docs_output_dir,
        '--forbid-path', meson.project_source_root(),
        '--forbid-path', meson.project_build_root(),
      ],
      depends: [html_docs_target],
      suite: 'contracts',
    )
  endif
endif
