// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "internal.h"

#include <array>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace pkgsource::detail {
namespace {

bool valid_env_name(const std::string& name) {
  if (name.empty()) return false;
  const auto first = static_cast<unsigned char>(name.front());
  if (!(std::isupper(first) || first == '_')) return false;
  for (unsigned char c : name)
    if (!(std::isupper(c) || std::isdigit(c) || c == '_')) return false;
  return true;
}

void validate_environment(const std::map<std::string, std::string>& environment) {
  static const std::array<const char*, 14> forbidden = {
    "LD_PRELOAD", "LD_LIBRARY_PATH", "BASH_ENV", "ENV", "CDPATH", "IFS",
    "SHELLOPTS", "PYTHONPATH", "PERL5LIB", "RUBYOPT", "GCONV_PATH",
    "LOCPATH", "NLSPATH", "TMPDIR"
  };
  for (const auto& [name, value] : environment) {
    (void)value;
    if (!valid_env_name(name))
      throw error(error_code::invalid_request,
                  "invalid worker environment variable name: " + name);
    for (const char* blocked : forbidden)
      if (name == blocked)
        throw error(error_code::invalid_request,
                    "unsafe worker environment variable: " + name);
    if (name.rfind("LD_", 0) == 0 || name.rfind("PKGSOURCE_", 0) == 0)
      throw error(error_code::invalid_request,
                  "reserved worker environment variable: " + name);
    if (value.find('\0') != std::string::npos)
      throw error(error_code::invalid_request,
                  "NUL byte in worker environment value: " + name);
  }
}

std::pair<std::string, std::filesystem::path> current_identity() {
  std::array<char, 16384> storage{};
  struct passwd record{};
  struct passwd* result = nullptr;
  if (::getpwuid_r(::geteuid(), &record, storage.data(), storage.size(), &result) == 0 && result)
    return {result->pw_name ? result->pw_name : std::to_string(::geteuid()),
            result->pw_dir ? result->pw_dir : "/"};
  return {std::to_string(::geteuid()), "/"};
}

std::map<std::string, std::string> make_environment(
    const evaluation_policy& policy,
    const std::filesystem::path& working_directory) {
  validate_environment(policy.environment);
  std::map<std::string, std::string> environment = policy.environment;
  environment["PATH"] = "/usr/bin:/bin";
  environment["LANG"] = "C";
  environment["LC_ALL"] = "C";
  environment["TMPDIR"] = working_directory.string();
  if (policy.identity) {
    if (policy.identity->user.empty() || policy.identity->home.empty())
      throw error(error_code::invalid_request, "incomplete worker identity");
    environment["HOME"] = policy.identity->home.string();
    environment["USER"] = policy.identity->user;
    environment["LOGNAME"] = policy.identity->user;
  } else {
    const auto [user, home] = current_identity();
    environment["HOME"] = home.string();
    environment["USER"] = user;
    environment["LOGNAME"] = user;
  }
  return environment;
}

void set_nonblocking(int fd) {
  const int flags = ::fcntl(fd, F_GETFL);
  if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0)
    throw error(error_code::worker_failed, "cannot configure worker pipe");
}

void append_pipe(int fd, std::string& target, std::size_t limit, bool& open) {
  std::array<char, 8192> buffer{};
  for (;;) {
    const ssize_t count = ::read(fd, buffer.data(), buffer.size());
    if (count > 0) {
      if (target.size() + static_cast<std::size_t>(count) > limit)
        throw error(error_code::worker_failed, "worker output exceeded safety limit");
      target.append(buffer.data(), static_cast<std::size_t>(count));
      continue;
    }
    if (count == 0) { open = false; ::close(fd); return; }
    if (errno == EINTR) continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK) return;
    open = false; ::close(fd);
    throw error(error_code::worker_failed, "cannot read worker output");
  }
}

} // namespace

worker_result run_worker(const std::filesystem::path& worker,
                         const std::filesystem::path& working_directory,
                         const evaluation_policy& policy) {
  if (policy.file_creation_mask > 0777U)
    throw error(error_code::invalid_request, "invalid worker umask");
  std::error_code ec;
  const auto worker_path = std::filesystem::absolute(worker, ec);
  if (ec || !std::filesystem::is_regular_file(worker_path))
    throw error(error_code::invalid_request,
                "Pkgfile worker not found: " + worker.string());
  const auto env_map = make_environment(policy, working_directory);

  std::vector<std::string> env_storage;
  env_storage.reserve(env_map.size());
  for (const auto& [name, value] : env_map)
    env_storage.push_back(name + "=" + value);
  std::vector<char*> envp;
  envp.reserve(env_storage.size() + 1);
  for (auto& value : env_storage) envp.push_back(value.data());
  envp.push_back(nullptr);

  std::string shell = "/bin/sh";
  std::string argument0 = "sh";
  std::string root = ".";
  char* argv[] = {argument0.data(), const_cast<char*>(worker_path.c_str()),
                  root.data(), nullptr};

  int out_pipe[2]{-1, -1};
  int err_pipe[2]{-1, -1};
  if (::pipe2(out_pipe, O_CLOEXEC) != 0)
    throw error(error_code::worker_failed, "cannot create worker stdout pipe");
  if (::pipe2(err_pipe, O_CLOEXEC) != 0) {
    ::close(out_pipe[0]);
    ::close(out_pipe[1]);
    throw error(error_code::worker_failed, "cannot create worker stderr pipe");
  }

  const pid_t child = ::fork();
  if (child < 0) {
    ::close(out_pipe[0]); ::close(out_pipe[1]);
    ::close(err_pipe[0]); ::close(err_pipe[1]);
    throw error(error_code::worker_failed, "cannot fork Pkgfile worker");
  }
  if (child == 0) {
    if (::setpgid(0, 0) != 0) _exit(126);
    if (::dup2(out_pipe[1], STDOUT_FILENO) < 0 ||
        ::dup2(err_pipe[1], STDERR_FILENO) < 0)
      _exit(126);
    ::close(out_pipe[0]); ::close(out_pipe[1]);
    ::close(err_pipe[0]); ::close(err_pipe[1]);
    if (::chdir(working_directory.c_str()) != 0) _exit(126);
    ::umask(static_cast<mode_t>(policy.file_creation_mask));
    if (policy.identity) {
      if (::geteuid() == 0) {
        if (::setgroups(0, nullptr) != 0 ||
            ::setgid(policy.identity->gid) != 0 ||
            ::setuid(policy.identity->uid) != 0)
          _exit(126);
      } else if (::geteuid() != policy.identity->uid || ::getegid() != policy.identity->gid) {
        _exit(126);
      }
    }

    ::execve(shell.c_str(), argv, envp.data());
    _exit(126);
  }

  if (::setpgid(child, child) != 0 && errno != EACCES && errno != ESRCH) {
    ::close(out_pipe[0]); ::close(out_pipe[1]);
    ::close(err_pipe[0]); ::close(err_pipe[1]);
    (void)::kill(-child, SIGKILL);
    (void)::kill(child, SIGKILL);
    (void)::waitpid(child, nullptr, 0);
    throw error(error_code::worker_failed,
                "cannot isolate Pkgfile worker process group");
  }

  ::close(out_pipe[1]);
  ::close(err_pipe[1]);
  try {
    set_nonblocking(out_pipe[0]);
    set_nonblocking(err_pipe[0]);
  } catch (...) {
    ::close(out_pipe[0]); ::close(err_pipe[0]);
    (void)::kill(-child, SIGKILL);
    (void)::kill(child, SIGKILL);
    while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR) {}
    throw;
  }

  worker_result result;
  bool out_open = true;
  bool err_open = true;
  bool child_reaped = false;
  int child_status = 0;
  try {
    while (out_open || err_open) {
      pollfd fds[2] = {{out_pipe[0], static_cast<short>(out_open ? POLLIN | POLLHUP : 0), 0},
                       {err_pipe[0], static_cast<short>(err_open ? POLLIN | POLLHUP : 0), 0}};
      int status;
      do { status = ::poll(fds, 2, 100); } while (status < 0 && errno == EINTR);
      if (status < 0)
        throw error(error_code::worker_failed, "cannot poll worker output");
      if (out_open && (fds[0].revents & (POLLIN | POLLHUP | POLLERR)))
        append_pipe(out_pipe[0], result.stdout_data, 1024 * 1024, out_open);
      if (err_open && (fds[1].revents & (POLLIN | POLLHUP | POLLERR)))
        append_pipe(err_pipe[0], result.stderr_data, 4 * 1024 * 1024, err_open);

      if (!child_reaped) {
        pid_t waited;
        do { waited = ::waitpid(child, &child_status, WNOHANG); }
        while (waited < 0 && errno == EINTR);
        if (waited < 0)
          throw error(error_code::worker_failed,
                      "cannot wait for Pkgfile worker");
        if (waited == child) {
          child_reaped = true;
          (void)::kill(-child, SIGKILL);
        }
      }
    }
  } catch (...) {
    if (out_open) ::close(out_pipe[0]);
    if (err_open) ::close(err_pipe[0]);
    (void)::kill(-child, SIGKILL);
    (void)::kill(child, SIGKILL);
    if (!child_reaped)
      while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR) {}
    throw;
  }

  if (!child_reaped) {
    pid_t waited;
    do { waited = ::waitpid(child, &child_status, 0); }
    while (waited < 0 && errno == EINTR);
    if (waited < 0)
      throw error(error_code::worker_failed, "cannot wait for Pkgfile worker");
  }
  (void)::kill(-child, SIGKILL);
  if (WIFEXITED(child_status)) result.exit_status = WEXITSTATUS(child_status);
  else if (WIFSIGNALED(child_status)) result.exit_status = 128 + WTERMSIG(child_status);
  else result.exit_status = 125;
  return result;
}

} // namespace pkgsource::detail
