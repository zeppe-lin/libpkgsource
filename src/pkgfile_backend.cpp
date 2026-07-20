// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/pkgfile_backend.h>
#include "internal.h"

#include <charconv>
#include <filesystem>
#include <grp.h>
#include <limits>
#include <set>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PKGSOURCE_PKGFILE_WORKER
#define PKGSOURCE_PKGFILE_WORKER "/usr/libexec/pkgsource-pkgfile-worker"
#endif

namespace pkgsource {
namespace {

std::size_t parse_count(const std::string& value) {
  std::size_t count = 0;
  const auto parsed = std::from_chars(value.data(), value.data() + value.size(), count);
  if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
    throw error(error_code::malformed_worker_record,
                "Pkgfile worker returned an invalid source count");
  return count;
}

bool regular_entry(const std::shared_ptr<const detail::snapshot_state>& state,
                   const std::string& path) {
  const auto found = state->files.find(path);
  return found != state->files.end() &&
         found->second.type == detail::file_record::kind::regular;
}

void chown_snapshot(const std::shared_ptr<detail::snapshot_state>& state,
                    uid_t uid, gid_t gid) {
  for (const auto& [key, record] : state->files) {
    (void)key;
    const auto path = state->root / record.relative_path;
    if (::lchown(path.c_str(), uid, gid) != 0)
      throw error(error_code::snapshot_failed,
                  "cannot change snapshot execution ownership: " + path.string());
  }
  if (::lchown(state->root.c_str(), uid, gid) != 0)
    throw error(error_code::snapshot_failed,
                "cannot change snapshot root ownership");
}

class identity_lease final {
public:
  identity_lease(const std::shared_ptr<detail::snapshot_state>& state,
                 const std::optional<worker_identity>& identity)
      : state_(state), original_uid_(::geteuid()), original_gid_(::getegid()) {
    if (identity && ::geteuid() == 0 &&
        (identity->uid != original_uid_ || identity->gid != original_gid_)) {
      chown_snapshot(state_, identity->uid, identity->gid);
      changed_ = true;
      baseline_ = detail::snapshot_manifest(state_);
    }
    if (!changed_) baseline_ = state_->files;
  }
  ~identity_lease() {
    if (!changed_) return;
    try { chown_snapshot(state_, original_uid_, original_gid_); }
    catch (...) { /* root-owned cleanup remains best effort in a destructor */ }
  }
  [[nodiscard]] const std::map<std::string, detail::file_record>& baseline() const noexcept { return baseline_; }
  void restore() {
    if (!changed_) return;
    chown_snapshot(state_, original_uid_, original_gid_);
    changed_ = false;
  }
private:
  std::shared_ptr<detail::snapshot_state> state_;
  uid_t original_uid_{};
  gid_t original_gid_{};
  bool changed_{false};
  std::map<std::string, detail::file_record> baseline_;
};

std::vector<lifecycle_action> lifecycle(
    const std::shared_ptr<const detail::snapshot_state>& state) {
  static const std::pair<const char*, lifecycle_phase> names[] = {
    {"pre-install", lifecycle_phase::pre_install},
    {"post-install", lifecycle_phase::post_install},
    {"pre-remove", lifecycle_phase::pre_remove},
    {"post-remove", lifecycle_phase::post_remove},
  };
  std::vector<lifecycle_action> result;
  for (const auto& [name, phase] : names) {
    if (state->files.find(name) == state->files.end()) continue;
    result.emplace_back(phase, detail::make_captured_file(state, name));
  }
  return result;
}

std::vector<resource> resources(
    const std::shared_ptr<const detail::snapshot_state>& state) {
  std::vector<resource> result;
  if (state->files.find("README") != state->files.end())
    result.emplace_back(resource_kind::readme, resource_format::plain_text,
                        detail::make_captured_file(state, "README"));
  if (state->files.find("README.md") != state->files.end())
    result.emplace_back(resource_kind::readme, resource_format::markdown,
                        detail::make_captured_file(state, "README.md"));
  return result;
}

} // namespace

pkgfile_backend::pkgfile_backend() : worker_(PKGSOURCE_PKGFILE_WORKER) {}
pkgfile_backend::pkgfile_backend(std::filesystem::path worker)
  : worker_(std::move(worker)) {}
source_format pkgfile_backend::format() const noexcept { return source_format::pkgfile_v0; }

bool pkgfile_backend::probe(const source_location& location) const {
  std::error_code ec;
  const auto status = std::filesystem::symlink_status(location.path() / "Pkgfile", ec);
  return !ec && status.type() == std::filesystem::file_type::regular;
}

source_snapshot pkgfile_backend::inspect(const inspect_request& request) const {
  auto captured = detail::capture_source_tree(request);
  const auto state = captured.state;
  if (!regular_entry(state, "Pkgfile"))
    throw error(error_code::invalid_pkgfile, "missing or non-regular Pkgfile");

  identity_lease ownership(state, request.evaluation.identity);
  const auto worker = detail::run_worker(worker_, state->root, request.evaluation);
  detail::verify_snapshot_unchanged(state, ownership.baseline());
  ownership.restore();
  state->files = detail::snapshot_manifest(state);
  if (worker.exit_status != 0)
    throw error(error_code::worker_failed,
                "Pkgfile worker failed with status " +
                    std::to_string(worker.exit_status) +
                    (worker.stderr_data.empty() ? std::string{} :
                     ": " + worker.stderr_data));

  detail::verify_source_unchanged(request.location, captured.original_manifest);

  const auto fields = detail::split_nul_records(worker.stdout_data);
  if (fields.size() < 6 || fields[0] != "pkgsource-pkgfile/0" ||
      fields[4] != "build")
    throw error(error_code::malformed_worker_record,
                "Pkgfile worker returned an unsupported record");
  const std::size_t count = parse_count(fields[5]);
  if (count > std::numeric_limits<std::size_t>::max() - 6 ||
      fields.size() != 6 + count)
    throw error(error_code::malformed_worker_record,
                "Pkgfile worker returned an inconsistent source record");

  package_identity identity(fields[1], fields[2], fields[3]);
  std::error_code name_error;
  auto source_path = std::filesystem::canonical(request.location.path(), name_error);
  if (name_error)
    throw error(error_code::source_mutated,
                "cannot resolve source directory after inspection");
  const auto expected_name = source_path.filename().string();
  if (expected_name.empty() || identity.name() != expected_name)
    throw error(error_code::invalid_pkgfile,
                "Pkgfile name does not match source-directory basename");

  std::vector<std::string> declarations;
  declarations.reserve(count);
  for (std::size_t i = 0; i < count; ++i) declarations.push_back(fields[6 + i]);

  auto metadata = detail::parse_pkgfile_metadata(state->root / "Pkgfile");
  auto sources = detail::normalize_sources(declarations, state);
  auto exclusions = detail::parse_nostrip(state);
  auto actions = lifecycle(state);
  auto readmes = resources(state);

  std::optional<footprint_declaration> footprint;
  if (state->files.find(".footprint") != state->files.end())
    footprint.emplace(footprint_format::pkgfile_footprint_v0,
                      detail::make_captured_file(state, ".footprint"));

  build_architecture architecture = build_architecture::native;
  if (state->files.find(".32bit") != state->files.end()) {
    (void)detail::make_captured_file(state, ".32bit");
    architecture = build_architecture::legacy_32bit;
  }

  recipe_descriptor recipe(source_format::pkgfile_v0,
                           recipe_entry_point::build,
                           detail::make_captured_file(state, "Pkgfile"));
  build_description build(std::move(identity), std::move(metadata.metadata),
                          std::move(metadata.dependencies), std::move(sources),
                          std::move(recipe), std::move(actions),
                          std::move(readmes), std::move(exclusions),
                          std::move(footprint), architecture);

  detail::verify_snapshot_unchanged(state, state->files);
  detail::protect_snapshot(state);
  return source_snapshot(request.location, source_format::pkgfile_v0,
                         std::move(captured.fingerprint), std::move(build), state);
}

} // namespace pkgsource
