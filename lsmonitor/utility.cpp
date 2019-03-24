#include "utility.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <fstream>

std::string linux::getPwuser(uid_t uid)
{
  std::string value;
  struct passwd pwd{};
  struct passwd *result = NULL; // NULL is intentional
  long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1)
    bufsize = 16384;
  std::vector<char> pwdData(bufsize);
  int err = getpwuid_r(uid, &pwd, pwdData.data(), pwdData.size(), &result);
  if (!err && result)
  {
    value = pwd.pw_name;
  }
  else if (err)
  {
    spdlog::warn("Unable get username of '{0}': {1} - {2}", uid, err, ::strerror(err));
  }

  return value;
}

std::string linux::getPwgroup(gid_t gid)
{
  std::string value;
  struct group pwd{};
  struct group *result = NULL; // NULL is intentional
  long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1)
    bufsize = 16384;
  std::vector<char> pwdData(bufsize);
  int err = getgrgid_r(gid, &pwd, pwdData.data(), pwdData.size(), &result);
  if (!err && result)
  {
    value = pwd.gr_name;
  }
  else if (err)
  {
    spdlog::warn("Unable get groupname of '{0}': {1} - {2}", gid, err, ::strerror(err));
  }

  return value;
}

std::string linux::getPidComm(pid_t pid)
{
  std::string comm("no_process");
  std::ifstream commFile(fmt::format("/proc/{0}/comm", pid).c_str());
  if (commFile.is_open() && !std::getline(commFile, comm))
    comm = "error";
  return comm;
}

std::string linux::getFdPath(int fd)
{
  std::string fdPath = fmt::format("/proc/self/fd/{0}", fd);
  std::string file("error");
  std::vector<char> path(PATH_MAX);
  path[PATH_MAX - 1] = 0;

  ssize_t pathLen = readlink(fdPath.c_str(), path.data(), path.size() - 2);
  if (pathLen > 0)
  {
    file.assign(path.data(), pathLen);
  }
  else
  {
    int err = errno;
    spdlog::warn("Unable to get link of '{0}': {1} - {2}", fdPath.c_str(), err, ::strerror(err));
  }

  return file;
}
