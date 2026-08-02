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
      dependencies: [libpkgsource_codec_dep, libcrypto_dep],
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

  source_contracts = {
    'codec-contract': 'contracts/check_codec_contract.sh',
    'core-boundary': 'contracts/check_core_boundary.sh',
    'doxygen-contract': 'contracts/check_doxygen_contract.sh',
    'documentation-contract': 'contracts/check_documentation_contract.sh',
    'provider-boundary': 'contracts/check_provider_boundary.sh',
    'release-metadata': 'contracts/check_release_metadata.sh',
    'test-structure': 'contracts/check_test_structure.sh',
  }
  foreach test_name, program_path : source_contracts
    contract = find_program(program_path)
    test(test_name, contract, args: [meson.project_source_root()])
  endforeach

  pkgconfig_metadata = find_program('contracts/pkgconfig_metadata_test.sh')
  test(
    'pkgconfig-metadata',
    pkgconfig_metadata,
    args: [meson.project_build_root(), meson.project_version()],
  )
endif
