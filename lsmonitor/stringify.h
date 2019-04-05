#pragma once

#include "lsprobe_event.h"
#include "fanotify_event.h"

#include "fmt/format.h"

#include <stlab/concurrency/channel.hpp>
#include <stlab/concurrency/default_executor.hpp>

#include <memory>
#include <stdexcept>

namespace lsp
{
  struct stringify
  {
    std::string _str{};
    stlab::process_state_scheduled _state = stlab::await_forever;

    template<typename T>
      void await(T event)
      {
	throw std::runtime_error("Unexpected type");
      }

    void await(std::unique_ptr<lsp::FileEvent> event)
    {
      spdlog::debug("{0}: {1}", __PRETTY_FUNCTION__, event->filename);
      _str = fmt::format("lsp: {0} : pid[{1}] : cred [{2}({3}):{4}({5})] : op[{6}] : {7}"
	    , event->process.c_str()
	    , event->pcred.tgid
	    , event->user
	    , event->pcred.uid
	    , event->group
	    , event->pcred.gid
	    , static_cast<int>(event->code)
	    , event->filename.c_str()
	    );
      _state = stlab::yield_immediate;
    }

    void await(std::unique_ptr<fan::FileEvent> event)
    {
      spdlog::debug("{0}: {1}", __PRETTY_FUNCTION__, event->filename);
      _str = fmt::format("fan: {0} : pid[{1}] : cred [{2}:{3}] : op[{4}] : {5}"
	    , event->process.c_str()
	    , event->pid
	    , event->uid
	    , event->gid
	    , static_cast<int>(event->code)
	    , event->filename.c_str()
	  );
      _state = stlab::yield_immediate;
    }

    std::string yield()
    {
      std::string event = std::move(_str);
      _state = stlab::await_forever;
      _str.clear();
      spdlog::debug("{0}: {1}", __PRETTY_FUNCTION__, event);
      return event;
    }

    auto state() const
    {
      return _state;
    }

    void set_error(std::exception_ptr error)
    {
      try
      {
	if (error)
	  std::rethrow_exception(error);
      }
      catch (const std::exception& e)
      {
	spdlog::critical("{0} : {1}", __PRETTY_FUNCTION__, e.what());
	throw;
      }
    }
  };
}
