#pragma once

#include "lsp_event.h"
#include "utility.h"

#include "spdlog/spdlog.h"

#include <string>
#include <vector>
#include <memory>

#include <sys/types.h>

namespace lsp
{
  struct FileEvent
  {
    FileEvent(const lsp_event_t * event)
      : code(static_cast<lsp_event_code_t>(event->code))
      , pcred(event->pcred)
      , filename(
	  lsp_event_field_first_const(event)->value
	  )
      , process(
	  lsp_event_field_get_const(event, 1)->value
	  )
      , user(linux::getPwuser(static_cast<uid_t>(event->pcred.uid)))
      , group(linux::getPwgroup(static_cast<gid_t>(event->pcred.gid)))
    {
      ++count;
    }

    FileEvent() = default;
    FileEvent(FileEvent&&) = default;
    FileEvent(const FileEvent&) = default;
    FileEvent& operator=(FileEvent&&) = default;
    FileEvent& operator=(const FileEvent&) = default;
    ~FileEvent()
    {
      --count;
    };

    lsp_event_code_t code{};
    lsp_cred_t pcred{};
    std::string filename{};
    std::string process{};
    std::string user{};
    std::string group{};

    static std::atomic_int count;
  };
}
