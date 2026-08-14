// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(PKGSOURCE_BUILDING_LIBRARY)
#    define PKGSOURCE_API __declspec(dllexport)
#  else
#    define PKGSOURCE_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define PKGSOURCE_API __attribute__((visibility("default")))
#else
#  define PKGSOURCE_API
#endif
