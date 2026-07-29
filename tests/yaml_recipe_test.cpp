// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource-yaml/parser.h>
#include <libpkgsource/error.h>

#include <cassert>
#include <functional>
#include <string_view>

using namespace pkgsource;
using namespace pkgsource::yaml_adapter;

namespace {

template <typename Function>
void expect_yaml(yaml_error_code code, Function&& function,
                 std::string_view path = {})
{
  try {
    function();
    assert(false);
  } catch (const yaml_error& value) {
    assert(value.code() == code);
    assert(value.document() == "recipe.yml");
    assert(value.line() > 0);
    assert(value.column() > 0);
    if (!path.empty())
      assert(value.path() == path);
  }
}

template <typename Function>
void expect_core(error_code code, Function&& function)
{
  try {
    function();
    assert(false);
  } catch (const error& value) {
    assert(value.code() == code);
  }
}

profile_catalog profiles()
{
  return seal_profiles_yaml_v1(
      R"(format: zeppe-lin.profiles/1
profiles:
  compiler:
    members:
      - package: gcc
  toolchain:
    members:
      - profile: "@compiler"
      - package: binutils
)", source_origin("profiles.yml"));
}

const char* complete_recipe()
{
  return R"(format: zeppe-lin.recipe/1
package:
  name: example
  version: 1.2.3
  release: 1
  summary: Example package
  description: |
    Native example package.
  homepage: https://example.invalid/
  licenses:
    - GPL-3.0-or-later
requirements:
  build:
    - profile: "@toolchain"
    - package: pkg-config
  run:
    - package: libfoo
  check:
    - package: pkgcheck
  lifecycle:
    post-install:
      - package: desktop-file-utils
sources:
  - url: https://example.invalid/example-1.2.3.tar.xz
    name: example-1.2.3.tar.xz
    sha256: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
  - path: files/example.conf
    name: example.conf
    sha256: abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789
build:
  language: posix-shell
  script: |
    meson setup build
    meson compile -C build
    meson install -C build --destdir "$PKG"
lifecycle:
  post-install:
    language: posix-shell
    script: |
      update-desktop-database
architectures:
  build:
    - x86_64
  target:
    - x86_64
)";
}

const char* reordered_recipe()
{
  return R"(sources:
  - name: example.conf
    path: files/example.conf
    sha256: abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789
  - sha256: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
    url: https://example.invalid/example-1.2.3.tar.xz
    name: example-1.2.3.tar.xz
format: zeppe-lin.recipe/1
architectures:
  target: [x86_64]
  build: [x86_64]
lifecycle:
  post-install:
    script: |
      update-desktop-database
    language: posix-shell
build:
  script: |
    meson setup build
    meson compile -C build
    meson install -C build --destdir "$PKG"
  language: posix-shell
requirements:
  check: [{package: pkgcheck}]
  run: [{package: libfoo}]
  lifecycle:
    post-install: [{package: desktop-file-utils}]
  build:
    - package: pkg-config
    - profile: "@toolchain"
package:
  release: 1
  version: 1.2.3
  name: example
  licenses: [GPL-3.0-or-later]
  homepage: https://example.invalid/
  description: |
    Native example package.
  summary: Example package
)";
}

const char* complete_recipe_v2()
{
  return R"(format: zeppe-lin.recipe/2
package:
  name: checked
  version: 2.0
  release: 1
  summary: Checked package
  licenses: [MIT]
requirements:
  check:
    - package: pkgcheck
sources: []
build:
  language: posix-shell
  script: |
    meson setup build
    meson compile -C build
check:
  language: posix-shell
  script: |
    meson test -C build
)";
}

void test_complete_recipe()
{
  const profile_catalog catalog = profiles();
  parsed_recipe_document parsed = parse_recipe_yaml_v1(
      complete_recipe(), source_origin("recipe.yml"));
  assert(parsed.origin().document() == "recipe.yml");
  assert(parsed.declaration().release().package().name() == "example");
  assert(parsed.declaration().provenance().path() == "document");
  assert(parsed.declaration().requirements()[0].provenance().path()
         == "requirements.build[0]");
  assert(!parsed.declaration().check_program());

  source_snapshot snapshot = seal_recipe_yaml_v1(
      complete_recipe(), source_origin("recipe.yml"), catalog);
  const sealed_recipe& recipe = snapshot.recipe();
  assert(recipe.release().version_release() == "1.2.3-1");
  assert(recipe.metadata().description().has_value());
  assert(recipe.sources().size() == 2);
  assert(recipe.build_requirements().size() == 3);
  assert(recipe.run_requirements().size() == 1);
  assert(recipe.check_requirements().size() == 1);
  assert(!recipe.check_program());
  assert(recipe.lifecycle_requirements(
             lifecycle_action::post_install).size() == 1);
  assert(recipe.selected_build_profiles().size() == 1);
  assert(recipe.profile_closure().size() == 2);
  assert(recipe.lifecycle(lifecycle_action::post_install) != nullptr);
  assert(recipe.architectures().target()[0].name() == "x86_64");
  assert(snapshot.syntax() == source_syntax::recipe_yaml_v1);

  source_snapshot reordered = seal_recipe_yaml_v1(
      reordered_recipe(), source_origin("other.yml"), catalog);
  assert(snapshot.recipe().identity() == reordered.recipe().identity());
  assert(snapshot.identity() == reordered.identity());
}

void test_check_program_recipe()
{
  const profile_catalog catalog = profiles();
  parsed_recipe_document parsed = parse_recipe_yaml_v2(
      complete_recipe_v2(), source_origin("recipe.yml"));
  assert(parsed.declaration().check_program());
  assert(parsed.declaration().check_program()->material()
         == "meson test -C build\n");

  source_snapshot snapshot = seal_recipe_yaml_v2(
      complete_recipe_v2(), source_origin("recipe.yml"), catalog);
  assert(snapshot.syntax() == source_syntax::recipe_yaml_v2);
  assert(snapshot.recipe().check_program());
  assert(snapshot.recipe().check_requirements().size() == 1);

  source_snapshot no_requirements = seal_recipe_yaml_v2(
      R"(format: zeppe-lin.recipe/2
package:
  name: checked
  version: 2.0
  release: 1
  summary: Checked package
  licenses: [MIT]
requirements: {}
sources: []
build: {language: posix-shell, script: "true\n"}
check: {language: posix-shell, script: "true\n"}
)", source_origin("recipe.yml"), catalog);
  assert(no_requirements.recipe().check_program());
  assert(no_requirements.recipe().check_requirements().empty());
}

void test_schema_rejections()
{
  expect_yaml(yaml_error_code::unknown_key, [] {
    (void)parse_recipe_yaml_v1(
        "format: zeppe-lin.recipe/1\ncheck: {}\n",
        source_origin("recipe.yml"));
  }, "check");

  expect_yaml(yaml_error_code::invalid_value, [] {
    (void)parse_recipe_yaml_v2(
        "format: zeppe-lin.recipe/1\n",
        source_origin("recipe.yml"));
  }, "format");

  expect_yaml(yaml_error_code::unknown_key, [] {
    (void)parse_recipe_yaml_v2(
        R"(format: zeppe-lin.recipe/2
package:
  name: example
  version: 1
  release: 1
  summary: Example
  licenses: [MIT]
requirements: {}
sources: []
build: {language: posix-shell, script: echo}
check: {language: posix-shell, program: echo}
)", source_origin("recipe.yml"));
  }, "check.program");

  expect_yaml(yaml_error_code::unknown_key, [] {
    (void)parse_recipe_yaml_v1(
        "format: zeppe-lin.recipe/1\nunknown: true\n",
        source_origin("recipe.yml"));
  }, "unknown");

  expect_yaml(yaml_error_code::invalid_value, [] {
    (void)parse_recipe_yaml_v1(
        "format: zeppe-lin.recipe/2\n",
        source_origin("recipe.yml"));
  }, "format");

  expect_yaml(yaml_error_code::missing_key, [] {
    (void)parse_recipe_yaml_v1(
        "format: zeppe-lin.recipe/1\n",
        source_origin("recipe.yml"));
  }, "package");

  expect_yaml(yaml_error_code::invalid_value, [] {
    (void)parse_recipe_yaml_v1(
        "format: zeppe-lin.recipe/1\n"
        "package:\n"
        "  name: example\n"
        "  version: 1\n"
        "  release: 01\n"
        "  summary: Example\n"
        "  licenses: [MIT]\n"
        "requirements: {}\nsources: []\n"
        "build: {language: posix-shell, script: echo}\n",
        source_origin("recipe.yml"));
  }, "package.release");

  expect_yaml(yaml_error_code::invalid_type, [] {
    (void)parse_recipe_yaml_v1(
        "format: zeppe-lin.recipe/1\n"
        "package:\n"
        "  name: example\n"
        "  version: 1\n"
        "  release: 1\n"
        "  summary: Example\n"
        "  licenses: [MIT]\n"
        "requirements:\n"
        "  build: [gcc]\n"
        "sources: []\n"
        "build: {language: posix-shell, script: echo}\n",
        source_origin("recipe.yml"));
  }, "requirements.build[0]");

  expect_yaml(yaml_error_code::invalid_value, [] {
    (void)parse_recipe_yaml_v1(
        "format: zeppe-lin.recipe/1\n"
        "package:\n"
        "  name: example\n"
        "  version: 1\n"
        "  release: 1\n"
        "  summary: Example\n"
        "  licenses: [MIT]\n"
        "requirements:\n"
        "  lifecycle:\n"
        "    configure: []\n"
        "sources: []\n"
        "build: {language: posix-shell, script: echo}\n",
        source_origin("recipe.yml"));
  }, "requirements.lifecycle.configure");

  expect_yaml(yaml_error_code::invalid_value, [] {
    (void)parse_recipe_yaml_v1(
        "format: zeppe-lin.recipe/1\n"
        "package:\n"
        "  name: example\n"
        "  version: 1\n"
        "  release: 1\n"
        "  summary: Example\n"
        "  licenses: [MIT]\n"
        "requirements: {}\nsources: []\n"
        "build: {language: python, script: echo}\n",
        source_origin("recipe.yml"));
  }, "build.language");
}

void test_sealing_rejections()
{
  const profile_catalog catalog = profiles();
  expect_core(error_code::invalid_recipe, [&] {
    (void)seal_recipe_yaml_v1(
        R"(format: zeppe-lin.recipe/1
package:
  name: example
  version: 1
  release: 1
  summary: Example
  licenses: [MIT]
requirements:
  lifecycle:
    post-install:
      - package: desktop-file-utils
sources: []
build:
  language: posix-shell
  script: echo build
)", source_origin("recipe.yml"), catalog);
  });

  expect_core(error_code::invalid_recipe, [&] {
    (void)seal_recipe_yaml_v2(
        R"(format: zeppe-lin.recipe/2
package:
  name: example
  version: 1
  release: 1
  summary: Example
  licenses: [MIT]
requirements:
  check:
    - package: pkgcheck
sources: []
build: {language: posix-shell, script: "true\n"}
)", source_origin("recipe.yml"), catalog);
  });

  expect_core(error_code::duplicate_declaration, [&] {
    (void)seal_recipe_yaml_v1(
        R"(format: zeppe-lin.recipe/1
package:
  name: example
  version: 1
  release: 1
  summary: Example
  licenses: [MIT]
requirements: {}
sources:
  - {url: https://example.invalid/a, name: source.tar, sha256: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef}
  - {url: https://example.invalid/b, name: source.tar, sha256: abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789}
build:
  language: posix-shell
  script: echo build
)", source_origin("recipe.yml"), catalog);
  });
}

} // namespace

int main()
{
  test_complete_recipe();
  test_check_program_recipe();
  test_schema_rejections();
  test_sealing_rejections();
}
