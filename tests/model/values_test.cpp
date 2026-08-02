// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/model_fixture.h"

namespace test_support::model_fixture {

void test_values()
{
  package_release first(package_reference("example"), "1.2.3", 1);
  package_release second(package_reference("example"), "1.2.3", 1);
  package_release next(package_reference("example"), "1.2.3", 2);
  assert(first.identity() == second.identity());
  assert(first.identity() != next.identity());
  assert(first.version_release() == "1.2.3-1");

  package_metadata metadata("Example package",
                            "Long\ndescription",
                            "https://example.invalid",
                            {"MIT", "BSD-2-Clause"});
  assert(metadata.licenses().front() == "BSD-2-Clause");

  const std::string hash(64, 'a');
  source_input remote =
      source_input::remote("https://example.invalid/example.tar.xz",
                           "example.tar.xz",
                           digest(digest_algorithm::sha256, hash));
  source_input local =
      source_input::local("files/example.conf",
                          "example.conf",
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
        {architecture_reference("x86_64"), architecture_reference("x86_64")},
        {});
  });
  expect(error_code::invalid_source, [&] {
    (void)source_input::local(
        "../escape", "escape", digest(digest_algorithm::sha256, hash));
  });
}

} // namespace test_support::model_fixture

int main()
{
  test_support::model_fixture::test_values();
}
