#include "utility.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include <pwd.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <fstream>

std::string lsp::getUsername(uid_t uid)
{
  std::string user;
  struct passwd pwd{};
  struct passwd *result = NULL; // NULL is intentional
  long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1)
    bufsize = 16384;
  std::vector<char> pwdData(bufsize);
  int err = getpwuid_r(uid, &pwd, pwdData.data(), pwdData.size(), &result);
  if (!err && result)
  {
    user = pwd.pw_name;
  }
  else if (err)
  {
    spdlog::warn("{0}: {1} - {2}", __func__, err, ::strerror(err));
  }

  return user;
}

std::string lsp::getPidComm(pid_t pid)
{
  std::string comm("no_process");
  std::ifstream commFile(fmt::format("/proc/{0:u}/comm", pid).c_str());
  if (commFile.is_open() && !std::getline(commFile, comm))
    comm = "error";
  return comm;
}

std::string lsp::getFdPath(int fd)
{
  std::string file("no_file");
  std::ifstream fdFile(fmt::format("/proc/self/fd/{0:d}", fd).c_str());
  if (fdFile.is_open() && !std::getline(fdFile, file))
    file = "error";
  return file;
}
