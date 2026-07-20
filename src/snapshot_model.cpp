// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/snapshot.h>
#include <libpkgsource/error.h>
#include "internal.h"

#include <utility>

namespace pkgsource {
source_location::source_location(std::filesystem::path p) : path_(std::move(p)) {
  if (path_.empty()) throw error(error_code::invalid_request, "empty source location");
}
const std::filesystem::path& source_location::path() const noexcept { return path_; }

source_snapshot::source_snapshot(source_location o, source_format f, digest fp, build_description b,
                                 std::shared_ptr<const detail::snapshot_state> s)
 : origin_(std::move(o)), format_(f), fingerprint_(std::move(fp)), build_(std::move(b)), state_(std::move(s)) {}
const source_location& source_snapshot::origin() const noexcept { return origin_; }
source_format source_snapshot::format() const noexcept { return format_; }
const digest& source_snapshot::fingerprint() const noexcept { return fingerprint_; }
const build_description& source_snapshot::build() const noexcept { return build_; }
std::filesystem::path source_snapshot::native_root() const {
  if (!state_) throw error(error_code::snapshot_failed, "snapshot has no captured tree");
  return state_->root;
}
} // namespace pkgsource
