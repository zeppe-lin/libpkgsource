#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
workflow=$root/.github/workflows/ci.yml
require() {
  grep -F -- "$1" "$workflow" >/dev/null || {
    echo "missing CI contract: $1" >&2
    exit 1
  }
}
require 'e1f6dfd8cc4bfeb2f8da44345d8ec6368281c6e0'
require '57a10b166450dd0396d4d461d1d38352073a5a1e'
require '-Dplanner_adapter=enabled'
