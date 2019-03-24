#pragma once

#include <string>
#include <sys/types.h>

namespace linux
{
  std::string getPwuser(uid_t);
  std::string getPwgroup(gid_t);
  std::string getPidComm(pid_t);
  std::string getFdPath(int fd);
}
