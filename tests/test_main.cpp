// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/libpkgsource.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifndef TEST_WORKER
#error TEST_WORKER is required
#endif
#ifndef TEST_CORPUS
#error TEST_CORPUS is required
#endif

namespace fs = std::filesystem;

namespace {
struct temporary_directory final {
  fs::path path;
  temporary_directory() {
    std::string pattern = (fs::temp_directory_path() / "libpkgsource-test.XXXXXX").string();
    std::vector<char> bytes(pattern.begin(), pattern.end()); bytes.push_back('\0');
    char* made = ::mkdtemp(bytes.data());
    if (!made) throw std::runtime_error("mkdtemp failed");
    path = made;
  }
  ~temporary_directory() { std::error_code ec; fs::remove_all(path, ec); }
};

void write(const fs::path& path, const std::string& data, mode_t mode = 0644) {
  fs::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary);
  if (!output) throw std::runtime_error("cannot write " + path.string());
  output << data;
  output.close();
  ::chmod(path.c_str(), mode);
}


pkgsource::source_snapshot inspect(const fs::path& source,
                                   pkgsource::evaluation_policy policy = {},
                                   const fs::path& worker = TEST_WORKER) {
  pkgsource::pkgfile_backend backend(worker);
  return backend.inspect({pkgsource::source_location(source), std::nullopt,
                          std::move(policy)});
}

template<class F>
void expect_error(pkgsource::error_code code, F&& callable) {
  try { callable(); }
  catch (const pkgsource::error& e) {
    if (e.code() != code)
      throw std::runtime_error("wrong error code: " + std::string(e.what()));
    return;
  }
  throw std::runtime_error("expected libpkgsource error");
}

void make_minimal(const fs::path& root, const std::string& name = "sample") {
  const auto dir = root / name;
  fs::create_directories(dir);
  write(dir / "Pkgfile",
        "# Description: sample\nname=" + name +
        "\nversion=1\nrelease=1\nbuild() { :; }\n");
}

void test_model_invariants() {
  expect_error(pkgsource::error_code::invalid_pkgfile, [] {
    pkgsource::package_identity("", "1", "1");
  });
  expect_error(pkgsource::error_code::invalid_pkgfile, [] {
    pkgsource::package_identity("bad/name", "1", "1");
  });
  expect_error(pkgsource::error_code::invalid_request, [] {
    pkgsource::digest(pkgsource::digest_algorithm::md5, "not-md5");
  });
  expect_error(pkgsource::error_code::invalid_metadata, [] {
    pkgsource::descriptive_metadata(std::string{}, std::nullopt, std::nullopt, std::nullopt);
  });
  expect_error(pkgsource::error_code::invalid_pkgfile, [] {
    pkgsource::source_input("https://example.invalid/a",
        pkgsource::source_input_kind::remote, "a", std::nullopt,
        {pkgsource::digest(pkgsource::digest_algorithm::md5,
                          "00000000000000000000000000000000")},
        std::nullopt);
  });
  pkgsource::package_identity identity("ok", "1.0", "2");
  if (identity.version_release() != "1.0-2") throw std::runtime_error("version-release");
}

void test_complete_corpus() {
  const auto snapshot = inspect(fs::path(TEST_CORPUS) / "complete" / "demo");
  const auto& b = snapshot.build();
  if (snapshot.format() != pkgsource::source_format::pkgfile_v0) throw std::runtime_error("format");
  if (b.identity().name() != "demo" || b.identity().version() != "1.2.3" || b.identity().release() != "4")
    throw std::runtime_error("identity");
  if (!b.metadata().description() || *b.metadata().description() != "Complete corpus package")
    throw std::runtime_error("description");
  if (b.dependencies().size() != 2 || b.dependencies()[0].scope() != pkgsource::dependency_scope::build_and_run)
    throw std::runtime_error("dependencies");
  if (b.sources().size() != 2 || b.sources()[1].kind() != pkgsource::source_input_kind::recipe_local ||
      !b.sources()[1].local_file())
    throw std::runtime_error("sources");
  if (b.lifecycle_actions().size() != 4) throw std::runtime_error("lifecycle");
  if (b.resources().size() != 2) throw std::runtime_error("resources");
  if (b.strip_exclusions().size() != 2) throw std::runtime_error("nostrip");
  if (!b.footprint()) throw std::runtime_error("footprint");
  if (b.architecture() != pkgsource::build_architecture::legacy_32bit)
    throw std::runtime_error("32bit");
  if (!fs::exists(snapshot.native_root() / "files" / "aux.txt") ||
      !fs::is_symlink(snapshot.native_root() / "aux-link"))
    throw std::runtime_error("complete snapshot");
}

void test_digest_stability() {
  const auto source = fs::path(TEST_CORPUS) / "complete" / "demo";
  const auto first = inspect(source);
  const auto second = inspect(source);
  if (first.fingerprint().hex() != second.fingerprint().hex())
    throw std::runtime_error("unstable snapshot digest");
}

void test_trailing_separator() {
  const auto source = fs::path(TEST_CORPUS) / "minimal" / "minimal" / "";
  const auto snapshot = inspect(source);
  if (snapshot.build().identity().name() != "minimal")
    throw std::runtime_error("trailing separator changed basename");
}

void test_directory_mode_fingerprint() {
  temporary_directory temp;
  make_minimal(temp.path, "modes");
  const auto auxiliary = temp.path / "modes" / "auxiliary";
  fs::create_directory(auxiliary);
  if (::chmod(auxiliary.c_str(), 0777) != 0)
    throw std::runtime_error("cannot set first directory mode");
  const auto first = inspect(temp.path / "modes");
  if (::chmod(auxiliary.c_str(), 0755) != 0)
    throw std::runtime_error("cannot set second directory mode");
  const auto second = inspect(temp.path / "modes");
  if (first.fingerprint().hex() == second.fingerprint().hex())
    throw std::runtime_error("directory mode missing from snapshot digest");
}

void test_explicit_environment() {
  pkgsource::evaluation_policy policy;
  policy.environment["SAFE_VALUE"] = "7";
  const auto snapshot = inspect(fs::path(TEST_CORPUS) / "dynamic" / "dynamic", policy);
  if (snapshot.build().identity().version() != "1.7")
    throw std::runtime_error("explicit environment not applied");
}

void test_ambient_environment_is_ignored() {
  if (::setenv("SAFE_VALUE", "99", 1) != 0)
    throw std::runtime_error("setenv failed");
  try {
    const auto snapshot = inspect(fs::path(TEST_CORPUS) / "dynamic" / "dynamic");
    if (snapshot.build().identity().version() != "1.0")
      throw std::runtime_error("ambient environment leaked into worker");
  } catch (...) {
    ::unsetenv("SAFE_VALUE");
    throw;
  }
  ::unsetenv("SAFE_VALUE");
}

void test_explicit_worker_identity() {
  struct passwd* account = nullptr;
  if (::geteuid() == 0) account = ::getpwnam("nobody");
  if (!account) account = ::getpwuid(::geteuid());
  if (!account) throw std::runtime_error("cannot resolve worker identity");

  pkgsource::evaluation_policy policy;
  policy.identity = pkgsource::worker_identity{
      account->pw_uid, account->pw_gid,
      account->pw_name ? account->pw_name : std::to_string(account->pw_uid),
      account->pw_dir ? account->pw_dir : "/"};
  const auto snapshot = inspect(fs::path(TEST_CORPUS) / "minimal" / "minimal",
                                policy);
  if (snapshot.build().identity().name() != "minimal")
    throw std::runtime_error("explicit worker identity failed");
}

void test_unsafe_environment() {
  pkgsource::evaluation_policy policy;
  policy.environment["LD_PRELOAD"] = "/tmp/evil.so";
  expect_error(pkgsource::error_code::invalid_request, [&] {
    (void)inspect(fs::path(TEST_CORPUS) / "minimal" / "minimal", policy);
  });
}

void test_missing_and_invalid_identity() {
  temporary_directory temp;
  fs::create_directories(temp.path / "missing");
  expect_error(pkgsource::error_code::invalid_pkgfile, [&] { (void)inspect(temp.path / "missing"); });
  make_minimal(temp.path, "actual");
  write(temp.path / "actual" / "Pkgfile",
        "name=other\nversion=1\nrelease=1\nbuild() { :; }\n");
  expect_error(pkgsource::error_code::invalid_pkgfile, [&] { (void)inspect(temp.path / "actual"); });
  write(temp.path / "actual" / "Pkgfile",
        "name=actual\nversion=\nrelease=1\nbuild() { :; }\n");
  expect_error(pkgsource::error_code::worker_failed, [&] { (void)inspect(temp.path / "actual"); });
  write(temp.path / "actual" / "Pkgfile",
        "name=actual\nversion=1\nrelease=1\n");
  expect_error(pkgsource::error_code::worker_failed, [&] { (void)inspect(temp.path / "actual"); });
}

void test_metadata_errors() {
  temporary_directory temp;
  make_minimal(temp.path, "meta");
  write(temp.path / "meta" / "Pkgfile",
        "# Description broken\nname=meta\nversion=1\nrelease=1\nbuild() { :; }\n");
  expect_error(pkgsource::error_code::invalid_metadata, [&] { (void)inspect(temp.path / "meta"); });
  write(temp.path / "meta" / "Pkgfile",
        "# Description: one\n# Description: two\nname=meta\nversion=1\nrelease=1\nbuild() { :; }\n");
  expect_error(pkgsource::error_code::invalid_metadata, [&] { (void)inspect(temp.path / "meta"); });
  write(temp.path / "meta" / "Pkgfile",
        "# Depends on: alpha alpha\nname=meta\nversion=1\nrelease=1\nbuild() { :; }\n");
  expect_error(pkgsource::error_code::invalid_metadata, [&] { (void)inspect(temp.path / "meta"); });
  write(temp.path / "meta" / "Pkgfile",
        "# URLish prose is not metadata\nname=meta\nversion=1\nrelease=1\nbuild() { :; }\n");
  (void)inspect(temp.path / "meta");
}

void test_checksum_errors() {
  temporary_directory temp;
  const auto dir = temp.path / "sum";
  fs::create_directories(dir);
  write(dir / "Pkgfile", "name=sum\nversion=1\nrelease=1\nsource=a\nbuild() { :; }\n");
  write(dir / "a", "a");
  expect_error(pkgsource::error_code::invalid_checksum, [&] { (void)inspect(dir); });
  write(dir / ".md5sum", "bad  a\n");
  expect_error(pkgsource::error_code::invalid_checksum, [&] { (void)inspect(dir); });
  write(dir / ".md5sum", "00000000000000000000000000000000  a\n11111111111111111111111111111111  a\n");
  expect_error(pkgsource::error_code::invalid_checksum, [&] { (void)inspect(dir); });
  write(dir / ".md5sum", "00000000000000000000000000000000  a\n11111111111111111111111111111111  other\n");
  expect_error(pkgsource::error_code::invalid_checksum, [&] { (void)inspect(dir); });
}

void test_explicit_remote_local_name() {
  temporary_directory temp;
  const auto dir = temp.path / "run-one";
  fs::create_directories(dir);
  write(dir / "Pkgfile",
        "name=run-one\nversion=1.18\nrelease=1\n"
        "source=\"$name-$version.tar.gz::https://github.com/dustinkirkland/"
        "$name/archive/refs/tags/$version.tar.gz\n"
        "f33be88dfab3f14c556794970cb5eda2a80dc045.patch\"\n"
        "build() { :; }\n");
  write(dir / "f33be88dfab3f14c556794970cb5eda2a80dc045.patch", "patch\n");
  write(dir / ".md5sum",
        "3026be5a7dc822ca393f4bc654af36e6  "
        "f33be88dfab3f14c556794970cb5eda2a80dc045.patch\n"
        "2615201658339fab693ecf05b72f057e  run-one-1.18.tar.gz\n");

  const auto snapshot = inspect(dir);
  const auto& sources = snapshot.build().sources();
  if (sources.size() != 2)
    throw std::runtime_error("renamed source count");
  const auto& archive = sources[0];
  if (archive.kind() != pkgsource::source_input_kind::remote ||
      archive.local_name() != "run-one-1.18.tar.gz" ||
      !archive.locator() ||
      *archive.locator() !=
          "https://github.com/dustinkirkland/run-one/archive/refs/tags/1.18.tar.gz" ||
      archive.digests().size() != 1 ||
      archive.digests()[0].hex() != "2615201658339fab693ecf05b72f057e")
    throw std::runtime_error("renamed remote source normalization");

  write(dir / "Pkgfile",
        "name=run-one\nversion=1.18\nrelease=1\n"
        "source='../run-one.tar.gz::https://example.invalid/run-one.tar.gz'\n"
        "build() { :; }\n");
  write(dir / ".md5sum",
        "2615201658339fab693ecf05b72f057e  run-one.tar.gz\n");
  expect_error(pkgsource::error_code::invalid_pkgfile, [&] { (void)inspect(dir); });
}

void test_duplicate_source_names() {
  temporary_directory temp;
  const auto dir = temp.path / "dup";
  fs::create_directories(dir);
  write(dir / "Pkgfile",
        "name=dup\nversion=1\nrelease=1\nsource='https://a/x.tar https://b/x.tar'\nbuild() { :; }\n");
  write(dir / ".md5sum", "00000000000000000000000000000000  x.tar\n");
  expect_error(pkgsource::error_code::invalid_pkgfile, [&] { (void)inspect(dir); });
}

void test_nostrip_error() {
  temporary_directory temp;
  make_minimal(temp.path, "strip");
  write(temp.path / "strip" / ".nostrip", "[broken\n");
  expect_error(pkgsource::error_code::invalid_sidecar, [&] { (void)inspect(temp.path / "strip"); });
}

void test_symlink_and_path_escape() {
  temporary_directory temp;
  make_minimal(temp.path, "escape");
  fs::create_symlink("/etc/passwd", temp.path / "escape" / "bad-link");
  expect_error(pkgsource::error_code::unsafe_source_tree, [&] { (void)inspect(temp.path / "escape"); });
  fs::remove(temp.path / "escape" / "bad-link");
  write(temp.path / "escape" / "Pkgfile",
        "name=escape\nversion=1\nrelease=1\nsource=../outside\nbuild() { :; }\n");
  write(temp.path / "escape" / ".md5sum", "00000000000000000000000000000000  outside\n");
  expect_error(pkgsource::error_code::unsafe_source_tree, [&] { (void)inspect(temp.path / "escape"); });
}

void test_special_object_rejection() {
  temporary_directory temp;
  make_minimal(temp.path, "special");
  if (::mkfifo((temp.path / "special" / "fifo").c_str(), 0600) != 0)
    throw std::runtime_error("mkfifo failed");
  expect_error(pkgsource::error_code::unsupported_object, [&] { (void)inspect(temp.path / "special"); });
}

fs::path fake_worker(const temporary_directory& temp, const std::string& body) {
  const auto path = temp.path / "worker";
  write(path, "#!/bin/sh\n" + body, 0755);
  return path;
}

void test_worker_restores_source_directory() {
  temporary_directory temp;
  const auto dir = temp.path / "cwd";
  fs::create_directories(dir / "files");
  write(dir / "Pkgfile",
        "name=cwd\nversion=1\nrelease=1\nsource='*.patch'\n"
        "cd files\nbuild() { :; }\n");
  write(dir / "root.patch", "root\n");
  write(dir / "files" / "other.patch", "other\n");
  write(dir / ".md5sum",
        "00000000000000000000000000000000  root.patch\n");
  const auto snapshot = inspect(dir);
  if (snapshot.build().sources().size() != 1 ||
      snapshot.build().sources()[0].declaration() != "root.patch")
    throw std::runtime_error("worker did not restore source-directory cwd");
}

void test_worker_descendant_cleanup() {
  temporary_directory temp;
  const auto dir = temp.path / "background";
  fs::create_directories(dir);
  write(dir / "Pkgfile",
        "name=background\nversion=1\nrelease=1\n"
        "sleep 30 &\nbuild() { :; }\n");
  const auto started = std::chrono::steady_clock::now();
  (void)inspect(dir);
  const auto elapsed = std::chrono::steady_clock::now() - started;
  if (elapsed > std::chrono::seconds(5))
    throw std::runtime_error("worker descendant kept inspection pipes alive");
}

void test_worker_framing() {
  temporary_directory temp;
  make_minimal(temp.path, "frame");
  auto worker = fake_worker(temp, "/usr/bin/printf 'wrong\\0'\n");
  expect_error(pkgsource::error_code::malformed_worker_record, [&] {
    (void)inspect(temp.path / "frame", {}, worker);
  });
  worker = fake_worker(temp,
      "/usr/bin/printf '%s\\0' pkgsource-pkgfile/0 frame 1 1 build bad\n");
  expect_error(pkgsource::error_code::malformed_worker_record, [&] {
    (void)inspect(temp.path / "frame", {}, worker);
  });
  worker = fake_worker(temp,
      "/usr/bin/printf '%s\\0' pkgsource-pkgfile/0 frame 1 1 build 1\n");
  expect_error(pkgsource::error_code::malformed_worker_record, [&] {
    (void)inspect(temp.path / "frame", {}, worker);
  });
}

void test_snapshot_mutation_detection() {
  temporary_directory temp;
  make_minimal(temp.path, "mutate");
  const auto worker = fake_worker(temp,
      "printf '# mutation\\n' >> ./Pkgfile\n"
      "/usr/bin/printf '%s\\0' pkgsource-pkgfile/0 mutate 1 1 build 0\n");
  expect_error(pkgsource::error_code::source_mutated, [&] {
    (void)inspect(temp.path / "mutate", {}, worker);
  });
}

void test_original_mutation_detection() {
  temporary_directory temp;
  make_minimal(temp.path, "origin");
  const auto worker = fake_worker(temp,
      "printf '# mutation\\n' >> \"$TEST_ORIGIN/Pkgfile\"\n"
      "/usr/bin/printf '%s\\0' pkgsource-pkgfile/0 origin 1 1 build 0\n");
  pkgsource::evaluation_policy policy;
  policy.environment["TEST_ORIGIN"] = (temp.path / "origin").string();
  expect_error(pkgsource::error_code::source_mutated, [&] {
    (void)inspect(temp.path / "origin", policy, worker);
  });
}

void test_snapshot_lifetime() {
  fs::path root;
  pkgsource::captured_file program;
  {
    const auto snapshot = inspect(fs::path(TEST_CORPUS) / "minimal" / "minimal");
    root = snapshot.native_root();
    program = snapshot.build().recipe().program();
  }
  if (!fs::exists(root) || !fs::exists(program.native_path()))
    throw std::runtime_error("captured file did not retain snapshot lifetime");
  program = pkgsource::captured_file{};
  if (fs::exists(root)) throw std::runtime_error("snapshot tree lifetime leak");
}

struct test_case { const char* name; std::function<void()> run; };
}

int main() {
  const std::vector<test_case> tests = {
    {"model invariants", test_model_invariants},
    {"complete corpus", test_complete_corpus},
    {"digest stability", test_digest_stability},
    {"trailing separator", test_trailing_separator},
    {"directory mode digest", test_directory_mode_fingerprint},
    {"explicit environment", test_explicit_environment},
    {"ambient environment", test_ambient_environment_is_ignored},
    {"explicit worker identity", test_explicit_worker_identity},
    {"unsafe environment", test_unsafe_environment},
    {"identity validation", test_missing_and_invalid_identity},
    {"metadata errors", test_metadata_errors},
    {"checksum errors", test_checksum_errors},
    {"explicit remote local name", test_explicit_remote_local_name},
    {"duplicate source names", test_duplicate_source_names},
    {"nostrip error", test_nostrip_error},
    {"symlink and path escape", test_symlink_and_path_escape},
    {"special object rejection", test_special_object_rejection},
    {"worker source cwd", test_worker_restores_source_directory},
    {"worker descendant cleanup", test_worker_descendant_cleanup},
    {"worker framing", test_worker_framing},
    {"snapshot mutation", test_snapshot_mutation_detection},
    {"original mutation", test_original_mutation_detection},
    {"snapshot lifetime", test_snapshot_lifetime},
  };
  std::size_t failed = 0;
  for (const auto& test : tests) {
    try { test.run(); std::cout << "ok - " << test.name << '\n'; }
    catch (const std::exception& e) {
      ++failed; std::cerr << "not ok - " << test.name << ": " << e.what() << '\n';
    }
  }
  if (failed) std::cerr << failed << " test(s) failed\n";
  return failed == 0 ? 0 : 1;
}
