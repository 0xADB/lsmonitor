#pragma once

#include "spdlog/spdlog.h"

#include "utility.h"

#include <string>
#include <vector>
#include <memory>

#include <sys/types.h>
#include <sys/fanotify.h>
#include "lspredicate/ast.hpp"
#include "lspredicate/cmdl_expression.h"

namespace fan
{
  enum class EventCode
  {
    NONE = 0
    , OPEN = FAN_OPEN
    , CLOSE = FAN_CLOSE
  };

  struct FileEvent
  {
    FileEvent(const fanotify_event_metadata * fa)
      : code(
	  (fa->mask & FAN_OPEN ) ? EventCode::OPEN  :
	  (fa->mask & FAN_CLOSE) ? EventCode::CLOSE :
	  EventCode::NONE
	  )
      , pid(fa->pid)
      , uid(-1)
      , gid(getpgid(fa->pid))
      , filename(linux::getFdPath(fa->fd))
      , process(linux::getPidComm(fa->pid))
    {}

    FileEvent() = default;
    FileEvent(FileEvent&&) = default;
    FileEvent(const FileEvent&) = default;
    FileEvent& operator=(FileEvent&&) = default;
    FileEvent& operator=(const FileEvent&) = default;
    ~FileEvent() = default;

    std::string stringify() const;
    bool evaluate(lspredicate::ast::expression const& ast) const;

    EventCode code{};
    pid_t pid{};
    uid_t uid{};
    gid_t gid{};
    std::string filename{};
    std::string process{};
  };

} // lsp

namespace lsp
{
  namespace predicate
  {
    template<>
    bool evaluate<std::unique_ptr<fan::FileEvent>>(const std::unique_ptr<fan::FileEvent>& event, lspredicate::ast::expression const& ast);
  }
}
