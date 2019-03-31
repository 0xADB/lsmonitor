#pragma once

#include <string>
#include <sys/socket.h>
#include <cstring>

#include "utility.h"

namespace ctl
{
  enum class EventCode
  {
    NONE
    , DISABLE
    , ENABLE
  };

  struct ControlEvent
  {
    ControlEvent(const char * text, ssize_t size, struct ucred ucred)
      : code(
	  (!text || !size) ? EventCode::NONE :
	  (size == 3 && strncmp(text, "off", 3) == 0 ? EventCode::DISABLE :
	  (size == 2 && strncmp(text, "on" , 2) == 0 ? EventCode::ENABLE  :
	  (EventCode::NONE)))
	  )
      , pid(ucred.pid)
      , uid(ucred.uid)
      , gid(ucred.gid)
      , process(linux::getPidComm(ucred.pid))
      , user("n/a"/*linux::getPwgroup(ucred.uid)*/)
      , group("n/a"/*linux::getPwgroup(ucred.gid)*/) // FIXME: causes recursion on /etc/group
    {}

    EventCode code{};
    pid_t pid{};
    uid_t uid{};
    gid_t gid{};
    std::string process{};
    std::string user{};
    std::string group{};
  };

} // ctl
