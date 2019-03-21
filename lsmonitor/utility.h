#pragma once

#include <string>
#include <sys/types.h>

namespace lsp
{
  std::string getUsername(uid_t uid);
  std::string getPidComm(pid_t pid);
  std::string getFdPath(int fd);
}
