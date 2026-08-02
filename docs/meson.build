# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
project_docdir = get_option('datadir') / 'doc' / meson.project_name()

install_data(
  '../README.md',
  '../HISTORY.md',
  '../CONTRIBUTING.md',
  '../MAINTAINING.md',
  '../COPYING',
  '../COPYRIGHT',
  install_dir: project_docdir,
  install_tag: 'doc',
)

install_data(
  'architecture.md',
  'abi.md',
  'code-style.md',
  'testing.md',
  'manpage-markdown.md',
  'html.md',
  install_dir: project_docdir,
  install_tag: 'doc',
)

install_data(
  'protocols/source-records-v1.md',
  install_dir: project_docdir / 'protocols',
  install_tag: 'doc',
)

install_data(
  'history/3.0-authority-reset.md',
  install_dir: project_docdir / 'history',
  install_tag: 'doc',
)

install_data(
  'man/libpkgsource.3.md',
  'man/pkgsource_model.3.md',
  'man/pkgsource_profile.3.md',
  'man/pkgsource_recipe.3.md',
  'man/pkgsource_snapshot.3.md',
  'man/pkgsource_codec.3.md',
  install_dir: project_docdir / 'man',
  install_tag: 'doc',
)

install_data(
  'assets/house.css',
  'assets/doxygen-extra.css',
  install_dir: project_docdir / 'assets',
  install_tag: 'doc',
)

subdir('man')

html_docs_enabled = get_option('html_docs').enabled()
html_docs_target = []
html_docs_output_dir = ''

if html_docs_enabled
  pandoc = find_program('pandoc', required: true)
  doxygen = find_program('doxygen', required: true)
  html_docs_builder = find_program(
    meson.project_source_root() / 'tools/build-html-docs.py',
  )
  html_docs_checker = find_program(
    meson.project_source_root() / 'tools/check-html-docs.py',
  )
  html_docs_installer = find_program(
    meson.project_source_root() / 'tools/install-html-docs.py',
  )

  html_docs_output_dir = (
    meson.current_build_dir()
    / 'html'
    / meson.project_name()
    / meson.project_version()
  )
  html_docs_sources = files(
    '../README.md',
    '../HISTORY.md',
    '../CONTRIBUTING.md',
    '../MAINTAINING.md',
    '../COPYING',
    '../COPYRIGHT',
    '../Doxyfile',
    'architecture.md',
    'abi.md',
    'code-style.md',
    'testing.md',
    'manpage-markdown.md',
    'html.md',
    'protocols/source-records-v1.md',
    'history/3.0-authority-reset.md',
    'man/libpkgsource.3.md',
    'man/pkgsource_model.3.md',
    'man/pkgsource_profile.3.md',
    'man/pkgsource_recipe.3.md',
    'man/pkgsource_snapshot.3.md',
    'man/pkgsource_codec.3.md',
    'assets/house.css',
    'assets/doxygen-extra.css',
  )

  html_docs_target = custom_target(
    'html-docs',
    input: html_docs_sources,
    output: 'html-docs.stamp',
    command: [
      html_docs_builder,
      '--source-root', meson.project_source_root(),
      '--output-dir', html_docs_output_dir,
      '--project-version', meson.project_version(),
      '--pandoc', pandoc.full_path(),
      '--doxygen', doxygen.full_path(),
      '--checker', html_docs_checker.full_path(),
      '--stamp', '@OUTPUT@',
    ],
    build_by_default: true,
  )

  meson.add_install_script(
    html_docs_installer,
    html_docs_target,
    html_docs_output_dir,
    (
      get_option('datadir')
      / 'htmldocs'
      / meson.project_name()
      / meson.project_version()
    ),
    install_tag: 'html-docs',
  )
endif
