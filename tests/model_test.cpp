// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/libpkgsource.h>

#include <cassert>
#include <functional>
#include <string>
#include <vector>

using namespace pkgsource;

namespace {

template <typename Function>
void expect(error_code code, Function&& function)
{
  try {
    function();
    assert(false);
  } catch (const error& value) {
    assert(value.code() == code);
  }
}

void test_identity_domains()
{
  package_reference package("pkg-config");
  profile_reference profile("@toolchain");
  architecture_reference architecture("x86_64");
  assert(package.name() == "pkg-config");
  assert(profile.name() == "@toolchain");
  assert(architecture.name() == "x86_64");

  expect(error_code::invalid_identity, [] { package_reference value("Pkg"); });
  expect(error_code::invalid_identity, [] { profile_reference value("toolchain"); });
  expect(error_code::invalid_identity, [] { architecture_reference value("x86/64"); });
}

void test_scopes_and_subjects()
{
  const requirement_scope build = requirement_scope::build();
  const requirement_scope run = requirement_scope::run();
  const requirement_scope check = requirement_scope::check();
  const requirement_scope lifecycle =
      requirement_scope::lifecycle(lifecycle_action::post_install);
  assert(build.kind() == requirement_scope_kind::build && !build.action());
  assert(run.kind() == requirement_scope_kind::run && !run.action());
  assert(check.kind() == requirement_scope_kind::check && !check.action());
  assert(lifecycle.kind() == requirement_scope_kind::lifecycle);
  assert(lifecycle.action() == lifecycle_action::post_install);

  requirement_subject exact(package_reference("libfoo"));
  requirement_subject group(profile_reference("@build-base"));
  assert(exact.kind() == requirement_subject_kind::package);
  assert(exact.package().name() == "libfoo");
  assert(group.kind() == requirement_subject_kind::profile);
  assert(group.profile().name() == "@build-base");
  expect(error_code::invalid_request, [&] { (void)exact.profile(); });
}

void test_values()
{
  package_release first(package_reference("example"), "1.2.3", 1);
  package_release second(package_reference("example"), "1.2.3", 1);
  package_release next(package_reference("example"), "1.2.3", 2);
  assert(first.identity() == second.identity());
  assert(first.identity() != next.identity());
  assert(first.version_release() == "1.2.3-1");

  package_metadata metadata("Example package", "Long\ndescription",
                            "https://example.invalid", {"MIT", "BSD-2-Clause"});
  assert(metadata.licenses().front() == "BSD-2-Clause");

  const std::string hash(64, 'a');
  source_input remote = source_input::remote(
      "https://example.invalid/example.tar.xz", "example.tar.xz",
      digest(digest_algorithm::sha256, hash));
  source_input local = source_input::local(
      "files/example.conf", "example.conf",
      digest(digest_algorithm::sha256, hash));
  assert(remote.kind() == source_input_kind::remote);
  assert(local.kind() == source_input_kind::local);

  program script(program_language::posix_shell, "echo build\n");
  assert(script.content_digest().hex().size() == 64);
  lifecycle_program post(lifecycle_action::post_install, script);
  assert(post.action() == lifecycle_action::post_install);

  architecture_requirements architectures(
      {architecture_reference("x86_64")},
      {architecture_reference("aarch64"), architecture_reference("x86_64")});
  assert(architectures.target().front().name() == "aarch64");

  expect(error_code::duplicate_declaration, [] {
    architecture_requirements value(
        {architecture_reference("x86_64"), architecture_reference("x86_64")}, {});
  });
  expect(error_code::invalid_source, [&] {
    (void)source_input::local("../escape", "escape", digest(digest_algorithm::sha256, hash));
  });
}

} // namespace

int main()
{
  test_identity_domains();
  test_scopes_and_subjects();
  test_values();
}
