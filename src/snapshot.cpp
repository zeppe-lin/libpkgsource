// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "internal.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>

namespace pkgsource::detail {
namespace {

class descriptor final {
public:
  explicit descriptor(int value = -1) noexcept : value_(value) {}
  ~descriptor() { if (value_ >= 0) (void)::close(value_); }
  descriptor(const descriptor&) = delete;
  descriptor& operator=(const descriptor&) = delete;
  descriptor(descriptor&& other) noexcept : value_(other.value_) { other.value_ = -1; }
  [[nodiscard]] int get() const noexcept { return value_; }
private:
  int value_;
};

[[noreturn]] void fail_fs(const std::string& operation,
                          const std::filesystem::path& path) {
  throw error(error_code::filesystem_failed,
              operation + " '" + path.string() + "': " + std::strerror(errno));
}

std::string hex(const unsigned char* data, std::size_t size) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string result(size * 2, '0');
  for (std::size_t i = 0; i < size; ++i) {
    result[2 * i] = digits[data[i] >> 4U];
    result[2 * i + 1] = digits[data[i] & 0x0fU];
  }
  return result;
}

class sha256_context final {
public:
  sha256_context() : context_(EVP_MD_CTX_new()) {
    if (!context_ || EVP_DigestInit_ex(context_, EVP_sha256(), nullptr) != 1)
      throw error(error_code::snapshot_failed, "cannot initialize SHA-256");
  }
  ~sha256_context() { EVP_MD_CTX_free(context_); }
  void update(const void* data, std::size_t size) {
    if (EVP_DigestUpdate(context_, data, size) != 1)
      throw error(error_code::snapshot_failed, "cannot update SHA-256");
  }
  void update(std::string_view value) { update(value.data(), value.size()); }
  digest finish() {
    unsigned char output[EVP_MAX_MD_SIZE]{};
    unsigned int size = 0;
    if (EVP_DigestFinal_ex(context_, output, &size) != 1 || size != 32)
      throw error(error_code::snapshot_failed, "cannot finish SHA-256");
    return digest(digest_algorithm::sha256, hex(output, size));
  }
private:
  EVP_MD_CTX* context_;
};

std::pair<std::int64_t, std::int64_t> mtime_of(const struct stat& st) {
  return {static_cast<std::int64_t>(st.st_mtim.tv_sec),
          static_cast<std::int64_t>(st.st_mtim.tv_nsec)};
}
std::pair<std::int64_t, std::int64_t> ctime_of(const struct stat& st) {
  return {static_cast<std::int64_t>(st.st_ctim.tv_sec),
          static_cast<std::int64_t>(st.st_ctim.tv_nsec)};
}

std::string readlink_value(const std::filesystem::path& path,
                           std::uintmax_t expected_size) {
  std::size_t capacity = static_cast<std::size_t>(std::min<std::uintmax_t>(
      std::max<std::uintmax_t>(expected_size + 1, 256), 1024 * 1024));
  for (;;) {
    std::vector<char> buffer(capacity);
    const ssize_t count = ::readlink(path.c_str(), buffer.data(), buffer.size());
    if (count < 0) fail_fs("cannot read symbolic link", path);
    if (static_cast<std::size_t>(count) < buffer.size())
      return std::string(buffer.data(), static_cast<std::size_t>(count));
    if (capacity >= 1024 * 1024)
      throw error(error_code::unsafe_source_tree,
                  "symbolic-link target is too large: " + path.string());
    capacity *= 2;
  }
}

bool path_within(const std::filesystem::path& root,
                 const std::filesystem::path& candidate) {
  auto r = root.begin();
  auto c = candidate.begin();
  for (; r != root.end(); ++r, ++c) {
    if (c == candidate.end() || *r != *c) return false;
  }
  return true;
}

void validate_symlink(const std::filesystem::path& root,
                      const file_record& record) {
  const std::filesystem::path target(record.symlink_target);
  if (target.empty() || target.is_absolute())
    throw error(error_code::unsafe_source_tree,
                "absolute or empty symbolic-link target: " +
                    record.relative_path.string());
  const auto lexical = (record.relative_path.parent_path() / target).lexically_normal();
  if (lexical.empty() || lexical.is_absolute())
    throw error(error_code::unsafe_source_tree,
                "symbolic-link target escapes source tree: " +
                    record.relative_path.string());
  for (const auto& component : lexical) {
    if (component == "..")
      throw error(error_code::unsafe_source_tree,
                  "symbolic-link target escapes source tree: " +
                      record.relative_path.string());
  }
  std::error_code ec;
  const auto resolved = std::filesystem::canonical(root / record.relative_path, ec);
  if (ec)
    throw error(error_code::unsafe_source_tree,
                "dangling or cyclic symbolic link: " +
                    record.relative_path.string());
  const auto canonical_root = std::filesystem::canonical(root, ec);
  if (ec || !path_within(canonical_root, resolved))
    throw error(error_code::unsafe_source_tree,
                "symbolic-link target escapes source tree: " +
                    record.relative_path.string());
}

digest hash_regular_fd(int fd, const std::filesystem::path& path) {
  sha256_context hash;
  std::array<unsigned char, 64 * 1024> buffer{};
  for (;;) {
    ssize_t count = ::read(fd, buffer.data(), buffer.size());
    if (count > 0) {
      hash.update(buffer.data(), static_cast<std::size_t>(count));
      continue;
    }
    if (count == 0) break;
    if (errno == EINTR) continue;
    fail_fs("cannot read regular file", path);
  }
  return hash.finish();
}

file_record record_from_stat(const std::filesystem::path& relative,
                             const struct stat& st) {
  file_record result;
  result.relative_path = relative;
  result.mode = static_cast<std::uint32_t>(st.st_mode & 07777U);
  result.size = static_cast<std::uintmax_t>(st.st_size);
  result.device = static_cast<std::uint64_t>(st.st_dev);
  result.inode = static_cast<std::uint64_t>(st.st_ino);
  const auto mt = mtime_of(st);
  const auto ct = ctime_of(st);
  result.mtime_seconds = mt.first;
  result.mtime_nanoseconds = mt.second;
  result.ctime_seconds = ct.first;
  result.ctime_nanoseconds = ct.second;
  if (S_ISDIR(st.st_mode)) result.type = file_record::kind::directory;
  else if (S_ISREG(st.st_mode)) result.type = file_record::kind::regular;
  else if (S_ISLNK(st.st_mode)) result.type = file_record::kind::symlink;
  else throw error(error_code::unsupported_object,
                   "unsupported filesystem object: " + relative.string());
  return result;
}

std::map<std::string, file_record> scan_tree(const std::filesystem::path& root,
                                             bool content) {
  struct stat st{};
  if (::lstat(root.c_str(), &st) != 0) fail_fs("cannot inspect source root", root);
  if (!S_ISDIR(st.st_mode))
    throw error(error_code::invalid_request, "source location is not a directory: " + root.string());

  std::map<std::string, file_record> records;
  std::function<void(const std::filesystem::path&)> walk;
  walk = [&](const std::filesystem::path& relative) {
    const auto directory = root / relative;
    DIR* raw = ::opendir(directory.c_str());
    if (!raw) fail_fs("cannot open source directory", directory);
    std::unique_ptr<DIR, int(*)(DIR*)> handle(raw, ::closedir);
    std::vector<std::string> names;
    errno = 0;
    while (dirent* entry = ::readdir(handle.get())) {
      std::string name(entry->d_name);
      if (name == "." || name == "..") continue;
      if (name.empty() || name.find('/') != std::string::npos)
        throw error(error_code::unsafe_source_tree,
                    "unsafe directory entry name in " + directory.string());
      names.push_back(std::move(name));
      errno = 0;
    }
    if (errno != 0) fail_fs("cannot enumerate source directory", directory);
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
      const auto rel = relative / name;
      const auto path = root / rel;
      struct stat lst{};
      if (::lstat(path.c_str(), &lst) != 0) fail_fs("cannot inspect source entry", path);
      auto rec = record_from_stat(rel, lst);
      if (rec.type == file_record::kind::regular && content) {
        descriptor fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
        if (fd.get() < 0) fail_fs("cannot open source file", path);
        struct stat before{};
        if (::fstat(fd.get(), &before) != 0) fail_fs("cannot inspect source file", path);
        const digest d = hash_regular_fd(fd.get(), path);
        struct stat after{};
        if (::fstat(fd.get(), &after) != 0) fail_fs("cannot reinspect source file", path);
        if (before.st_dev != after.st_dev || before.st_ino != after.st_ino ||
            before.st_size != after.st_size ||
            before.st_mtim.tv_sec != after.st_mtim.tv_sec ||
            before.st_mtim.tv_nsec != after.st_mtim.tv_nsec ||
            before.st_ctim.tv_sec != after.st_ctim.tv_sec ||
            before.st_ctim.tv_nsec != after.st_ctim.tv_nsec)
          throw error(error_code::source_mutated,
                      "source file mutated while hashing: " + path.string());
        rec = record_from_stat(rel, after);
        rec.content_digest = d;
      } else if (rec.type == file_record::kind::symlink) {
        rec.symlink_target = readlink_value(path, rec.size);
      }
      const auto key = rel.generic_string();
      if (!records.emplace(key, std::move(rec)).second)
        throw error(error_code::unsafe_source_tree, "duplicate source-tree path: " + key);
      if (records.at(key).type == file_record::kind::directory) walk(rel);
    }
  };
  walk({});
  for (const auto& [key, record] : records)
    if (record.type == file_record::kind::symlink) validate_symlink(root, record);
  return records;
}

bool equivalent(const file_record& a, const file_record& b, bool identity) {
  if (a.type != b.type || a.relative_path != b.relative_path || a.mode != b.mode ||
      a.size != b.size || a.symlink_target != b.symlink_target ||
      a.mtime_seconds != b.mtime_seconds || a.mtime_nanoseconds != b.mtime_nanoseconds ||
      a.ctime_seconds != b.ctime_seconds || a.ctime_nanoseconds != b.ctime_nanoseconds)
    return false;
  if (identity && (a.device != b.device || a.inode != b.inode)) return false;
  if (a.content_digest.has_value() != b.content_digest.has_value()) return false;
  if (a.content_digest && (a.content_digest->algorithm() != b.content_digest->algorithm() ||
                           a.content_digest->hex() != b.content_digest->hex())) return false;
  return true;
}

void compare_manifests(const std::map<std::string, file_record>& expected,
                       const std::map<std::string, file_record>& actual,
                       bool identity,
                       const std::string& what) {
  if (expected.size() != actual.size())
    throw error(error_code::source_mutated, what + " entry set changed during inspection");
  auto a = expected.begin();
  auto b = actual.begin();
  for (; a != expected.end(); ++a, ++b) {
    if (a->first != b->first || !equivalent(a->second, b->second, identity))
      throw error(error_code::source_mutated,
                  what + " mutated during inspection at " + a->first);
  }
}

void write_all(int fd, const unsigned char* data, std::size_t size,
               const std::filesystem::path& path) {
  std::size_t offset = 0;
  while (offset < size) {
    const ssize_t count = ::write(fd, data + offset, size - offset);
    if (count > 0) { offset += static_cast<std::size_t>(count); continue; }
    if (count < 0 && errno == EINTR) continue;
    fail_fs("cannot write snapshot file", path);
  }
}

void copy_regular(const std::filesystem::path& source,
                  const std::filesystem::path& destination,
                  const file_record& expected) {
  descriptor input(::open(source.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
  if (input.get() < 0) fail_fs("cannot open source file", source);
  struct stat before{};
  if (::fstat(input.get(), &before) != 0) fail_fs("cannot inspect source file", source);
  descriptor output(::open(destination.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600));
  if (output.get() < 0) fail_fs("cannot create snapshot file", destination);
  sha256_context hash;
  std::array<unsigned char, 64 * 1024> buffer{};
  for (;;) {
    ssize_t count = ::read(input.get(), buffer.data(), buffer.size());
    if (count > 0) {
      hash.update(buffer.data(), static_cast<std::size_t>(count));
      write_all(output.get(), buffer.data(), static_cast<std::size_t>(count), destination);
      continue;
    }
    if (count == 0) break;
    if (errno == EINTR) continue;
    fail_fs("cannot read source file", source);
  }
  if (::fchmod(output.get(), static_cast<mode_t>(expected.mode & 0777U)) != 0)
    fail_fs("cannot preserve snapshot file mode", destination);
  struct stat after{};
  if (::fstat(input.get(), &after) != 0) fail_fs("cannot reinspect source file", source);
  const digest copied = hash.finish();
  if (!expected.content_digest || copied.hex() != expected.content_digest->hex() ||
      before.st_dev != after.st_dev || before.st_ino != after.st_ino ||
      before.st_size != after.st_size ||
      before.st_mtim.tv_sec != after.st_mtim.tv_sec ||
      before.st_mtim.tv_nsec != after.st_mtim.tv_nsec ||
      before.st_ctim.tv_sec != after.st_ctim.tv_sec ||
      before.st_ctim.tv_nsec != after.st_ctim.tv_nsec)
    throw error(error_code::source_mutated,
                "source file mutated while copying: " + source.string());
}

digest fingerprint(const std::map<std::string, file_record>& records) {
  sha256_context hash;
  hash.update("libpkgsource-snapshot/1\0", 24);
  for (const auto& [path, record] : records) {
    const char kind = record.type == file_record::kind::directory ? 'd' :
                      record.type == file_record::kind::regular ? 'f' : 'l';
    hash.update(&kind, 1);
    hash.update(path);
    const char zero = '\0'; hash.update(&zero, 1);
    const std::string mode = std::to_string(record.mode & 0777U);
    hash.update(mode); hash.update(&zero, 1);
    if (record.type == file_record::kind::regular) {
      hash.update(std::to_string(record.size)); hash.update(&zero, 1);
      hash.update(record.content_digest->hex()); hash.update(&zero, 1);
    } else if (record.type == file_record::kind::symlink) {
      hash.update(record.symlink_target); hash.update(&zero, 1);
    }
  }
  return hash.finish();
}

std::filesystem::path make_snapshot_root(const inspect_request& request,
                                         const std::filesystem::path& source) {
  std::filesystem::path parent = request.snapshot_parent.value_or(
      std::filesystem::temp_directory_path());
  std::error_code ec;
  parent = std::filesystem::absolute(parent, ec);
  if (ec) throw error(error_code::snapshot_failed, "cannot resolve snapshot parent");
  std::filesystem::create_directories(parent, ec);
  if (ec) throw error(error_code::snapshot_failed,
                      "cannot create snapshot parent: " + parent.string());
  const auto canonical_source = std::filesystem::canonical(source, ec);
  if (ec) throw error(error_code::invalid_request, "cannot resolve source location");
  const auto canonical_parent = std::filesystem::canonical(parent, ec);
  if (ec) throw error(error_code::snapshot_failed, "cannot resolve snapshot parent");
  if (path_within(canonical_source, canonical_parent))
    throw error(error_code::invalid_request,
                "snapshot parent must not be inside source tree");
  std::string pattern = (parent / ".libpkgsource.XXXXXX").string();
  std::vector<char> bytes(pattern.begin(), pattern.end());
  bytes.push_back('\0');
  char* made = ::mkdtemp(bytes.data());
  if (!made) fail_fs("cannot create private snapshot", parent);
  if (::chmod(made, 0700) != 0) {
    const int saved = errno; std::filesystem::remove_all(made, ec); errno = saved;
    fail_fs("cannot protect private snapshot", made);
  }
  return made;
}

} // namespace

snapshot_state::~snapshot_state() {
  std::error_code ignored;
  if (root.empty()) return;
  for (auto it = std::filesystem::recursive_directory_iterator(
           root, std::filesystem::directory_options::skip_permission_denied, ignored);
       !ignored && it != std::filesystem::recursive_directory_iterator(); ++it) {
    if (it->is_directory(ignored))
      std::filesystem::permissions(it->path(), std::filesystem::perms::owner_all,
                                   std::filesystem::perm_options::add, ignored);
  }
  std::filesystem::permissions(root, std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::add, ignored);
  std::filesystem::remove_all(root, ignored);
}

digest sha256_file(const std::filesystem::path& path) {
  descriptor fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
  if (fd.get() < 0) fail_fs("cannot open file for hashing", path);
  struct stat st{};
  if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode))
    throw error(error_code::filesystem_failed, "file is not regular: " + path.string());
  return hash_regular_fd(fd.get(), path);
}

std::string read_text_file(const std::filesystem::path& path) {
  descriptor fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
  if (fd.get() < 0) fail_fs("cannot open text file", path);
  struct stat st{};
  if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode))
    throw error(error_code::filesystem_failed, "text file is not regular: " + path.string());
  if (st.st_size > 16 * 1024 * 1024)
    throw error(error_code::invalid_sidecar, "text sidecar is too large: " + path.string());
  std::string result;
  result.reserve(static_cast<std::size_t>(st.st_size));
  std::array<char, 64 * 1024> buffer{};
  for (;;) {
    ssize_t count = ::read(fd.get(), buffer.data(), buffer.size());
    if (count > 0) { result.append(buffer.data(), static_cast<std::size_t>(count)); continue; }
    if (count == 0) break;
    if (errno == EINTR) continue;
    fail_fs("cannot read text file", path);
  }
  if (result.find('\0') != std::string::npos)
    throw error(error_code::invalid_sidecar, "NUL byte in text sidecar: " + path.string());
  return result;
}

captured_snapshot capture_source_tree(const inspect_request& request) {
  std::error_code ec;
  const auto source = std::filesystem::absolute(request.location.path(), ec);
  if (ec) throw error(error_code::invalid_request, "cannot resolve source location");
  const auto original = scan_tree(source, true);
  const auto root = make_snapshot_root(request, source);
  auto state = std::make_shared<snapshot_state>();
  state->root = root;
  try {
    for (const auto& [key, record] : original) {
      const auto destination = root / record.relative_path;
      if (record.type == file_record::kind::directory) {
        if (::mkdir(destination.c_str(), 0700) != 0)
          fail_fs("cannot create snapshot directory", destination);
        if (::chmod(destination.c_str(), 0700) != 0)
          fail_fs("cannot prepare snapshot directory", destination);
      } else if (record.type == file_record::kind::regular) {
        copy_regular(source / record.relative_path, destination, record);
      } else {
        if (::symlink(record.symlink_target.c_str(), destination.c_str()) != 0)
          fail_fs("cannot create snapshot symbolic link", destination);
      }
    }
    std::vector<std::pair<std::filesystem::path, std::uint32_t>> directories;
    for (const auto& [key, record] : original) {
      (void)key;
      if (record.type == file_record::kind::directory)
        directories.emplace_back(root / record.relative_path, record.mode);
    }
    std::sort(directories.begin(), directories.end(), [](const auto& left,
                                                         const auto& right) {
      return std::distance(left.first.begin(), left.first.end()) >
             std::distance(right.first.begin(), right.first.end());
    });
    for (const auto& [directory, mode] : directories)
      if (::chmod(directory.c_str(), static_cast<mode_t>(mode & 0777U)) != 0)
        fail_fs("cannot preserve snapshot directory mode", directory);

    const auto current_source = scan_tree(source, true);
    compare_manifests(original, current_source, true, "source tree");
    state->files = scan_tree(root, true);
    return captured_snapshot{state, fingerprint(state->files), original};
  } catch (...) {
    state.reset();
    throw;
  }
}

void verify_source_unchanged(const source_location& source,
                             const std::map<std::string, file_record>& expected) {
  std::error_code ec;
  const auto root = std::filesystem::absolute(source.path(), ec);
  if (ec) throw error(error_code::source_mutated, "cannot resolve source location");
  compare_manifests(expected, scan_tree(root, true), true, "source tree");
}

void verify_snapshot_unchanged(const std::shared_ptr<snapshot_state>& state,
                               const std::map<std::string, file_record>& expected) {
  compare_manifests(expected, scan_tree(state->root, true), true, "captured snapshot");
}

std::map<std::string, file_record> snapshot_manifest(
    const std::shared_ptr<snapshot_state>& state) {
  return scan_tree(state->root, true);
}

void protect_snapshot(const std::shared_ptr<snapshot_state>& state) {
  std::vector<std::filesystem::path> directories;
  for (const auto& [key, record] : state->files) {
    const auto path = state->root / record.relative_path;
    if (record.type == file_record::kind::regular) {
      mode_t mode = 0400;
      if ((record.mode & 0111U) != 0) mode |= 0100;
      if (::chmod(path.c_str(), mode) != 0) fail_fs("cannot seal snapshot file", path);
    } else if (record.type == file_record::kind::directory) {
      directories.push_back(path);
    }
  }
  std::sort(directories.begin(), directories.end(), [](const auto& a, const auto& b) {
    return std::distance(a.begin(), a.end()) > std::distance(b.begin(), b.end());
  });
  for (const auto& directory : directories)
    if (::chmod(directory.c_str(), 0500) != 0)
      fail_fs("cannot seal snapshot directory", directory);
  if (::chmod(state->root.c_str(), 0500) != 0)
    fail_fs("cannot seal snapshot root", state->root);
}

captured_file make_captured_file(const std::shared_ptr<const snapshot_state>& state,
                                 const std::filesystem::path& relative_path) {
  if (relative_path.empty() || relative_path.is_absolute() ||
      relative_path != relative_path.lexically_normal())
    throw error(error_code::invalid_sidecar, "unsafe captured-file path");
  const auto key = relative_path.generic_string();
  const auto found = state->files.find(key);
  if (found == state->files.end() || found->second.type != file_record::kind::regular ||
      !found->second.content_digest)
    throw error(error_code::invalid_sidecar,
                "captured file is missing or not regular: " + key);
  return captured_file(state, relative_path, *found->second.content_digest,
                       found->second.mode, found->second.size);
}

} // namespace pkgsource::detail
