# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  core_behavior_tests = {
    'model-identity': 'model/identity_test.cpp',
    'model-requirements': 'model/requirements_test.cpp',
    'model-values': 'model/values_test.cpp',
    'profile-expansion': 'profiles/expansion_test.cpp',
    'profile-requirements': 'profiles/requirements_test.cpp',
    'profile-identity': 'profiles/identity_test.cpp',
    'profile-duplicates': 'profiles/duplicate_test.cpp',
    'profile-failures': 'profiles/failures_test.cpp',
    'recipe-content': 'recipes/content_test.cpp',
    'recipe-normalization': 'recipes/normalization_test.cpp',
    'recipe-identity': 'recipes/identity_test.cpp',
    'recipe-check-program': 'recipes/check_program_test.cpp',
    'recipe-program-digest': 'recipes/program_digest_test.cpp',
    'recipe-failures': 'recipes/failures_test.cpp',
  }
  foreach test_name, source_file : core_behavior_tests
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_dep,
      install: false,
    )
    test(test_name, executable_target)
  endforeach

  codec_behavior_tests = {
    'codec-golden-vectors': 'codec/golden_vectors_test.cpp',
    'codec-profile-catalog': 'codec/profile_catalog_test.cpp',
    'codec-source-snapshot': 'codec/source_snapshot_test.cpp',
    'codec-envelope-failures': 'codec/envelope_failures_test.cpp',
    'codec-identity-failures': 'codec/identity_failures_test.cpp',
    'codec-value-failures': 'codec/value_failures_test.cpp',
    'codec-canonicality': 'codec/canonicality_test.cpp',
  }
  foreach test_name, source_file : codec_behavior_tests
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_codec_dep,
      install: false,
    )
    test(test_name, executable_target)
  endforeach

  sha256_provider_test = executable(
    'sha256-provider-test',
    [
      'internal/sha256_test.cpp',
      '../internal/sha256_openssl.cpp',
    ],
    include_directories: repository_internal_inc,
    dependencies: libcrypto_dep,
    install: false,
  )
  test('sha256-provider', sha256_provider_test)

  core_public_headers = {
    'public-core-umbrella': 'public/core_umbrella_header_test.cpp',
    'public-error-header': 'public/error_header_test.cpp',
    'public-identity-header': 'public/identity_header_test.cpp',
    'public-model-header': 'public/model_header_test.cpp',
    'public-profile-header': 'public/profile_header_test.cpp',
    'public-recipe-header': 'public/recipe_header_test.cpp',
    'public-snapshot-header': 'public/snapshot_header_test.cpp',
  }
  foreach test_name, source_file : core_public_headers
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_dep,
      install: false,
    )
    test(test_name, executable_target)
  endforeach

  codec_public_headers = {
    'public-codec-component': 'public/codec_header_test.cpp',
    'public-codec-umbrella': 'public/codec_umbrella_header_test.cpp',
  }
  foreach test_name, source_file : codec_public_headers
    executable_target = executable(
      test_name,
      source_file,
      dependencies: libpkgsource_codec_dep,
      install: false,
    )
    test(test_name, executable_target)
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
    )
  endforeach

  pkgconfig_metadata = find_program('contracts/pkgconfig_metadata_test.sh')
  test(
    'pkgconfig-metadata',
    pkgconfig_metadata,
    args: [meson.project_build_root(), meson.project_version()],
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
    )
  endif
endif
