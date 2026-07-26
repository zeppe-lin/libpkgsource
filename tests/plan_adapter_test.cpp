// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource-plan/adapter.h>
#include <libpkgsource/pkgfile_backend.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

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
  temporary_directory()
  {
    std::string pattern =
        (fs::temp_directory_path() / "libpkgsource-plan-test.XXXXXX").string();
    std::vector<char> bytes(pattern.begin(), pattern.end());
    bytes.push_back('\0');
    char* made = ::mkdtemp(bytes.data());
    if (made == nullptr)
      throw std::runtime_error("mkdtemp failed");
    path = made;
  }
  ~temporary_directory()
  {
    std::error_code error;
    fs::remove_all(path, error);
  }
};

[[nodiscard]] pkgsource::source_snapshot
inspect(const fs::path& path)
{
  pkgsource::pkgfile_backend backend(TEST_WORKER);
  return backend.inspect({pkgsource::source_location(path), std::nullopt, {}});
}

void replace_text(const fs::path& path,
                  const std::string& before,
                  const std::string& after)
{
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw std::runtime_error("cannot open fixture");
  std::string text((std::istreambuf_iterator<char>(input)),
                   std::istreambuf_iterator<char>());
  const std::size_t position = text.find(before);
  if (position == std::string::npos)
    throw std::runtime_error("fixture text not found");
  text.replace(position, before.size(), after);
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  output.write(text.data(), static_cast<std::streamsize>(text.size()));
}

void check_complete_projection()
{
  auto projection = pkgsource::plan_adapter::project_candidate(
      inspect(fs::path(TEST_CORPUS) / "complete" / "demo"));
  const auto& candidate = projection.candidate();
  const auto& release = candidate.release();
  const auto& control = candidate.control_projection();

  if (release.name() != "demo" || release.version() != "1.2.3" ||
      release.release() != "4")
    throw std::runtime_error("release projection");
  if (control.runtime_dependencies().size() != 2 ||
      control.runtime_dependencies()[0].expression() != "alpha" ||
      control.runtime_dependencies()[1].expression() != "beta")
    throw std::runtime_error("dependency projection");
  if (control.removal_lifecycle().size() != 2)
    throw std::runtime_error("removal lifecycle projection");
  if (control.removal_lifecycle()[0].format() != "text/x-shellscript" ||
      control.removal_lifecycle()[1].format() != "text/x-shellscript")
    throw std::runtime_error("lifecycle format");
  if (control.removal_lifecycle()[0].material() != "#!/bin/sh\nexit 0\n" ||
      control.removal_lifecycle()[1].material() != "#!/bin/sh\nexit 0\n")
    throw std::runtime_error("lifecycle material");
  if (control.target_profile().size() != 1 ||
      control.target_profile()[0].name() != "pkgsource.build-architecture" ||
      control.target_profile()[0].value() != "legacy_32bit")
    throw std::runtime_error("architecture projection");

  const auto repeated = pkgsource::plan_adapter::project_candidate(
      inspect(fs::path(TEST_CORPUS) / "complete" / "demo"));
  if (candidate.identity() != repeated.candidate().identity() ||
      release.identity() != repeated.candidate().release().identity())
    throw std::runtime_error("projection identity stability");
}

void check_identity_domains()
{
  temporary_directory temp;
  const fs::path source = temp.path / "demo";
  fs::copy(fs::path(TEST_CORPUS) / "complete" / "demo", source,
           fs::copy_options::recursive | fs::copy_options::copy_symlinks);

  const auto original = pkgsource::plan_adapter::project_candidate(inspect(source));
  replace_text(source / "README", "Complete", "Unrelated");
  const auto readme_changed =
      pkgsource::plan_adapter::project_candidate(inspect(source));

  if (original.source_fingerprint().hex() ==
      readme_changed.source_fingerprint().hex())
    throw std::runtime_error("source fingerprint ignored README change");
  if (original.candidate().identity() != readme_changed.candidate().identity())
    throw std::runtime_error("candidate identity included unrelated source bytes");
  if (original.candidate().release().identity() !=
      readme_changed.candidate().release().identity())
    throw std::runtime_error("release identity included unrelated source bytes");

  replace_text(source / "post-remove", "exit 0", "exit 1");
  const auto control_changed =
      pkgsource::plan_adapter::project_candidate(inspect(source));
  if (readme_changed.candidate().identity() == control_changed.candidate().identity())
    throw std::runtime_error("candidate identity ignored control change");
  if (readme_changed.candidate().release().identity() !=
      control_changed.candidate().release().identity())
    throw std::runtime_error("release identity changed with control");

  replace_text(source / "Pkgfile", "version=1.2.3", "version=1.2.4");
  const auto release_changed =
      pkgsource::plan_adapter::project_candidate(inspect(source));
  if (control_changed.candidate().release().identity() ==
      release_changed.candidate().release().identity())
    throw std::runtime_error("release identity ignored release change");
}

} // namespace

int main()
{
  check_complete_projection();
  check_identity_domains();
}
