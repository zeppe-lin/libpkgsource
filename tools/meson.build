# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
pkgsource_inspect = disabler()
if get_option('reference_tools').enabled() or get_option('tests').enabled()
  pkgsource_inspect = executable(
    'pkgsource-inspect',
    'pkgsource-inspect.cpp',
    dependencies: libpkgsource_dep,
    install: false,
  )
endif
