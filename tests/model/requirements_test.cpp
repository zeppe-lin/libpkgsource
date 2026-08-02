// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/model_fixture.h"

namespace test_support::model_fixture {

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
  expect(error_code::invalid_request, [&] {
    (void)exact.profile();
  });
}

} // namespace test_support::model_fixture

int main()
{
  test_support::model_fixture::test_scopes_and_subjects();
}
