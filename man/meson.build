# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

scdoc = find_program('scdoc', required: get_option('man_pages'))
if scdoc.found()
  foreach page : [
    ['libpkgsource.3.scd', 'libpkgsource.3'],
    ['pkgsource_model.3.scd', 'pkgsource_model.3'],
    ['pkgsource_profile.3.scd', 'pkgsource_profile.3'],
    ['pkgsource_recipe.3.scd', 'pkgsource_recipe.3'],
    ['pkgsource_snapshot.3.scd', 'pkgsource_snapshot.3'],
    ['pkgsource_plan_adapter.3.scd', 'pkgsource_plan_adapter.3'],
  ]
    custom_target(
      page[1],
      input: page[0],
      output: page[1],
      command: [scdoc],
      feed: true,
      capture: true,
      install: true,
      install_dir: join_paths(get_option('mandir'), 'man3'),
    )
  endforeach
endif
