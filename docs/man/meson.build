# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if not get_option('man_pages').disabled()
  foreach page : [
    'libpkgsource.3',
    'pkgsource_model.3',
    'pkgsource_profile.3',
    'pkgsource_recipe.3',
    'pkgsource_snapshot.3',
    'pkgsource_codec.3',
  ]
    configure_file(
      input: 'generated/' + page,
      output: page,
      copy: true,
      install: true,
      install_dir: get_option('mandir') / 'man3',
      install_tag: 'man',
    )
  endforeach
endif

pandoc = find_program('pandoc', required: false)
if pandoc.found()
  update_man_pages = find_program(
    meson.project_source_root() / 'tools/update-man-pages.sh',
  )
  run_target(
    'update-man-pages',
    command: [
      update_man_pages,
      '--write',
      pandoc.full_path(),
      meson.project_source_root(),
    ],
  )
  run_target(
    'check-man-pages',
    command: [
      update_man_pages,
      '--check',
      pandoc.full_path(),
      meson.project_source_root(),
    ],
  )
endif
