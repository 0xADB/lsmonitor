#include "lsprobe_event.h"
#include "spdlog/spdlog.h"

#include <pwd.h>
#include <errno.h>
#include <cstring>

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

