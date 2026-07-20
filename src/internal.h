// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgsource/backend.h>
#include <libpkgsource/error.h>
#include <libpkgsource/model.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pkgsource::detail {

struct file_record final {
  enum class kind { directory, regular, symlink } type;
  std::filesystem::path relative_path;
  std::uint32_t mode{};
  std::uintmax_t size{};
  std::uint64_t device{};
  std::uint64_t inode{};
  std::int64_t mtime_seconds{};
  std::int64_t mtime_nanoseconds{};
  std::int64_t ctime_seconds{};
  std::int64_t ctime_nanoseconds{};
  std::string symlink_target;
  std::optional<digest> content_digest;
};

struct snapshot_state final {
  std::filesystem::path root;
  std::map<std::string, file_record> files;
  ~snapshot_state();
};

struct captured_snapshot final {
  std::shared_ptr<snapshot_state> state;
  digest fingerprint;
  std::map<std::string, file_record> original_manifest;
};

captured_snapshot capture_source_tree(const inspect_request& request);
void verify_source_unchanged(const source_location& source,
                             const std::map<std::string, file_record>& expected);
void verify_snapshot_unchanged(const std::shared_ptr<snapshot_state>& state,
                               const std::map<std::string, file_record>& expected);
std::map<std::string, file_record> snapshot_manifest(
    const std::shared_ptr<snapshot_state>& state);
void protect_snapshot(const std::shared_ptr<snapshot_state>& state);

captured_file make_captured_file(
    const std::shared_ptr<const snapshot_state>& state,
    const std::filesystem::path& relative_path);

std::string read_text_file(const std::filesystem::path& path);
digest sha256_file(const std::filesystem::path& path);

struct worker_result final {
  int exit_status{};
  std::string stdout_data;
  std::string stderr_data;
};

worker_result run_worker(const std::filesystem::path& worker,
                         const std::filesystem::path& working_directory,
                         const evaluation_policy& policy);

struct metadata_result final {
  descriptive_metadata metadata;
  std::vector<dependency> dependencies;
};
metadata_result parse_pkgfile_metadata(const std::filesystem::path& pkgfile);

std::vector<std::string> split_nul_records(const std::string& data);
std::vector<source_input> normalize_sources(
    const std::vector<std::string>& declarations,
    const std::shared_ptr<const snapshot_state>& state);
std::vector<strip_exclusion> parse_nostrip(
    const std::shared_ptr<const snapshot_state>& state);

} // namespace pkgsource::detail
