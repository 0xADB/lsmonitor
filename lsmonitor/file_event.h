#pragma once

#include "event.h"
#include "lsp_event.h"
#include "utility.h"

#include <string>
#include <vector>
#include <memory>

#include <sys/types.h>
#include <sys/fanotify.h>

namespace fan
{
  struct FileEvent : public lsp::Event
  {
    FileEvent(const fanotify_event_metadata * fa)
      : code((fa->mask & FAN_OPEN) ?  LSP_EVENT_CODE_OPEN : LSP_EVENT_CODE_NONE)
      , pid(fa->pid)
      , uid(0)
      , gid(0)
      , filename(lsp::getFdPath(fa->fd))
      , process(lsp::getPidComm(fa->pid))
      , user()
    {
      spdlog::trace("{0}: [{1}] : [{2}] : [{3}]"
	  , __PRETTY_FUNCTION__
	  , process.c_str()
	  , user.c_str()
	  , filename.c_str()
	  );
    }

    FileEvent() = default;
    FileEvent(FileEvent&&) = default;
    FileEvent(const FileEvent&) = default;
    FileEvent& operator=(FileEvent&&) = default;
    FileEvent& operator=(const FileEvent&) = default;
    virtual ~FileEvent() final
    {
      spdlog::trace("{0}: [{1}] : [{2}] : [{3}]"
	  , __PRETTY_FUNCTION__
	  , process.c_str()
	  , user.c_str()
	  , filename.c_str()
	  );
    };

    virtual std::string toString() const final
    {
      return fmt::format("{0}: [{1}] : [{2}] : [{3}]"
	  , "FAN"
	  , process.c_str()
	  , user.c_str()
	  , filename.c_str()
	  );
    }

    lsp_event_code_t code{};
    pid_t pid{};
    uid_t uid{};
    gid_t gid{};
    std::string filename{};
    std::string process{};
    std::string user{};
  };
} // lsp
