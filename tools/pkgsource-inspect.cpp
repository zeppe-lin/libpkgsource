// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource/libpkgsource.h>

#include <iostream>
#include <optional>
#include <string>

namespace {
std::string json(const std::string& value) {
  std::string out = "\"";
  for (unsigned char c : value) {
    switch (c) {
      case '\"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          static constexpr char h[] = "0123456789abcdef";
          out += "\\u00"; out += h[c >> 4U]; out += h[c & 15U];
        } else out += static_cast<char>(c);
    }
  }
  out += '"';
  return out;
}
void optional_string(const std::optional<std::string>& value) {
  if (value) std::cout << json(*value); else std::cout << "null";
}
void file(const pkgsource::captured_file& value) {
  std::cout << "{\"path\":" << json(value.relative_path().generic_string())
            << ",\"sha256\":" << json(value.content_digest().hex())
            << ",\"mode\":" << value.original_mode()
            << ",\"size\":" << value.size() << "}";
}
}

int main(int argc, char** argv) {
  try {
    std::filesystem::path worker;
    std::filesystem::path source;
    if (argc == 2) source = argv[1];
    else if (argc == 4 && std::string(argv[1]) == "--worker") {
      worker = argv[2]; source = argv[3];
    } else {
      std::cerr << "usage: pkgsource-inspect [--worker PATH] PACKAGE_SOURCE\n";
      return 64;
    }
    pkgsource::pkgfile_backend backend = worker.empty()
      ? pkgsource::pkgfile_backend{}
      : pkgsource::pkgfile_backend{worker};
    const auto snapshot = backend.inspect({pkgsource::source_location(source), std::nullopt, {}});
    const auto& build = snapshot.build();
    const auto& metadata = build.metadata();
    std::cout << "{\n";
    std::cout << "  \"schema\":\"pkgsource-inspect/1\",\n";
    std::cout << "  \"format\":" << json(std::string(pkgsource::to_string(snapshot.format()))) << ",\n";
    std::cout << "  \"snapshot_sha256\":" << json(snapshot.fingerprint().hex()) << ",\n";
    std::cout << "  \"identity\":{\"name\":" << json(build.identity().name())
              << ",\"version\":" << json(build.identity().version())
              << ",\"release\":" << json(build.identity().release()) << "},\n";
    std::cout << "  \"metadata\":{\"description\":"; optional_string(metadata.description());
    std::cout << ",\"url\":"; optional_string(metadata.url());
    std::cout << ",\"packager\":"; optional_string(metadata.packager());
    std::cout << ",\"maintainer\":"; optional_string(metadata.maintainer());
    std::cout << "},\n";
    std::cout << "  \"dependencies\":[";
    for (std::size_t i = 0; i < build.dependencies().size(); ++i) {
      if (i) std::cout << ',';
      const auto& d = build.dependencies()[i];
      std::cout << "{\"name\":" << json(d.name()) << ",\"scope\":"
                << json(std::string(pkgsource::to_string(d.scope()))) << "}";
    }
    std::cout << "],\n  \"sources\":[";
    for (std::size_t i = 0; i < build.sources().size(); ++i) {
      if (i) std::cout << ',';
      const auto& s = build.sources()[i];
      std::cout << "{\"declaration\":" << json(s.declaration())
                << ",\"kind\":" << json(std::string(pkgsource::to_string(s.kind())))
                << ",\"local_name\":" << json(s.local_name()) << ",\"locator\":";
      optional_string(s.locator());
      std::cout << ",\"digests\":[";
      for (std::size_t j = 0; j < s.digests().size(); ++j) {
        if (j) std::cout << ',';
        std::cout << "{\"algorithm\":"
                  << json(std::string(pkgsource::to_string(s.digests()[j].algorithm())))
                  << ",\"hex\":" << json(s.digests()[j].hex()) << "}";
      }
      std::cout << "]";
      if (s.local_file()) { std::cout << ",\"local_file\":"; file(*s.local_file()); }
      std::cout << "}";
    }
    std::cout << "],\n  \"recipe\":{\"format\":"
              << json(std::string(pkgsource::to_string(build.recipe().format())))
              << ",\"entry_point\":"
              << json(std::string(pkgsource::to_string(build.recipe().entry_point())))
              << ",\"program\":";
    file(build.recipe().program());
    std::cout << "}";
    std::cout << ",\n  \"architecture\":" << json(std::string(pkgsource::to_string(build.architecture()))) << ",\n";
    std::cout << "  \"lifecycle_actions\":[";
    for (std::size_t i = 0; i < build.lifecycle_actions().size(); ++i) {
      if (i) std::cout << ',';
      const auto& a = build.lifecycle_actions()[i];
      std::cout << "{\"phase\":" << json(std::string(pkgsource::to_string(a.phase()))) << ",\"program\":";
      file(a.program()); std::cout << "}";
    }
    std::cout << "],\n  \"resources\":[";
    for (std::size_t i = 0; i < build.resources().size(); ++i) {
      if (i) std::cout << ',';
      const auto& resource = build.resources()[i];
      std::cout << "{\"kind\":"
                << json(std::string(pkgsource::to_string(resource.kind())))
                << ",\"format\":"
                << json(std::string(pkgsource::to_string(resource.format()))) << ",\"file\":";
      file(resource.file()); std::cout << "}";
    }
    std::cout << "],\n  \"strip_exclusions\":[";
    for (std::size_t i = 0; i < build.strip_exclusions().size(); ++i) {
      if (i) std::cout << ',';
      const auto& exclusion = build.strip_exclusions()[i];
      std::cout << "{\"syntax\":"
                << json(std::string(pkgsource::to_string(exclusion.syntax())))
                << ",\"pattern\":" << json(exclusion.pattern()) << "}";
    }
    std::cout << "],\n  \"footprint\":";
    if (build.footprint()) {
      std::cout << "{\"format\":"
                << json(std::string(pkgsource::to_string(build.footprint()->format())))
                << ",\"file\":";
      file(build.footprint()->file());
      std::cout << "}";
    } else std::cout << "null";
    std::cout << "\n}\n";
    return 0;
  } catch (const pkgsource::error& e) {
    std::cerr << "pkgsource-inspect: " << e.what() << '\n';
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "pkgsource-inspect: " << e.what() << '\n';
    return 1;
  }
}
