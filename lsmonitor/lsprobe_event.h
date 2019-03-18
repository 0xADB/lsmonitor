#pragma once

#include "lsp_event.h"

#include "spdlog/spdlog.h"

#include <string>
#include <vector>
#include <memory>

#include <sys/types.h>

namespace lsp
{
  std::string getUsername(uid_t uid);

  struct Event
  {
    virtual ~Event(){}
  };

  struct FileEvent : public Event
  {
    FileEvent(const lsp_event_t * lsp_event)
      : code(static_cast<lsp_event_code_t>(lsp_event->code))
      , pid(lsp_event->pid)
      , uid(lsp_event->uid)
      , gid(lsp_event->gid)
      , filename(
	  lsp_event_field_first_const(lsp_event)->value
	  , lsp_event_field_first_const(lsp_event)->size
	  )
      , process(
	  lsp_event_field_get_const(lsp_event, 1)->value
	  , lsp_event_field_get_const(lsp_event, 1)->size
	  )
      , user(getUsername(lsp_event->uid))
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

    lsp_event_code_t code{};
    pid_t pid{};
    uid_t uid{};
    gid_t gid{};
    std::string filename{};
    std::string process{};
    std::string user{};
  };
}
