#pragma once

#include "fanotify_event.h"
#include "lsprobe_event.h"
#include "argh.h"

namespace lsp
{
  struct CmdlSimplePredicate
  {
    argh::parser _cmdl{};
    pid_t _pid = static_cast<pid_t>(-1);
    uid_t _uid = static_cast<uid_t>(-1);
    gid_t _gid = static_cast<gid_t>(-1);

    CmdlSimplePredicate(const argh::parser& cmdl)
      : _cmdl(cmdl)
    {
      readIds();
    }

    CmdlSimplePredicate(argh::parser&& cmdl)
      : _cmdl(std::move(cmdl))
    {
      readIds();
    }

    void print() const
    {
      if (_pid != static_cast<pid_t>(-1)) spdlog::debug("Found cmdl[\"pid\"]: {0}", _pid);
      if (_uid != static_cast<uid_t>(-1)) spdlog::debug("Found cmdl[\"uid\"]: {0}", _uid);
      if (_gid != static_cast<gid_t>(-1)) spdlog::debug("Found cmdl[\"gid\"]: {0}", _gid);
      if (_cmdl("file"         )) spdlog::debug("Found cmdl[\"file\"]: {0}", _cmdl("file").str());
      if (_cmdl("user"         )) spdlog::debug("Found cmdl[\"user\"]: {0}", _cmdl("user").str());
      if (_cmdl("group"        )) spdlog::debug("Found cmdl[\"group\"]: {0}", _cmdl("group").str());
      if (_cmdl("process"      )) spdlog::debug("Found cmdl[\"process\"]: {0}", _cmdl("process").str());
      if (_cmdl("file_contains")) spdlog::debug("Found cmdl[\"file_contains\"]: {0}", _cmdl("file_contains").str());
    }

    void readIds()
    {
      _cmdl("pid", -1) >> _pid;
      _cmdl("uid", -1) >> _uid;
      _cmdl("gid", -1) >> _gid;
      print();
    }

    bool operator()(const std::unique_ptr<lsp::FileEvent>& event) const
    {
      // TODO: auto pred = (AND(OR(1,2),NOT(1)));
      return (
	     (_pid == static_cast<pid_t>(-1) ? true : (event->pcred.tgid == _pid))
	  && (_uid == static_cast<uid_t>(-1) ? true : (event->pcred.uid == _uid))
	  && (_gid == static_cast<gid_t>(-1) ? true : (event->pcred.gid == _gid))
	  && (!_cmdl("file"         )        ? true : (event->filename == _cmdl("file").str()))
	  && (!_cmdl("user"         )        ? true : (event->user     == _cmdl("user").str()))
	  && (!_cmdl("group"        )        ? true : (event->group    == _cmdl("group").str()))
	  && (!_cmdl("process"      )        ? true : (event->process  == _cmdl("process").str()))
	  && (!_cmdl("file_contains")        ? true : (event->filename.find(_cmdl("file_contains").str()) != std::string::npos))
	  );
    }

    bool operator()(const std::unique_ptr<fan::FileEvent>& event) const
    {
      return (
	     (_pid == static_cast<pid_t>(-1) ? true : (event->pid == _pid))
	  && (_uid == static_cast<uid_t>(-1) ? true : (event->uid == _uid))
	  && (_gid == static_cast<gid_t>(-1) ? true : (event->gid == _gid))
	  && (!_cmdl("file"         )        ? true : (event->filename == _cmdl("file").str()))
	  && (!_cmdl("user"         )        ? true : (event->user     == _cmdl("user").str()))
	  && (!_cmdl("group"        )        ? true : (event->group    == _cmdl("group").str()))
	  && (!_cmdl("process"      )        ? true : (event->process  == _cmdl("process").str()))
	  && (!_cmdl("file_contains")        ? true : (event->filename.find(_cmdl("file_contains").str()) != std::string::npos))
	  );
    }
  };

  struct SameFilenamePredicate
  {
    template<typename L, typename R>
      bool operator()(const L& l, const R& r) const
      {
	spdlog::debug("comparing filenames: {0} == {1}", l->filename, r->filename);
	return (l->filename == r->filename);
      }
  };
}
