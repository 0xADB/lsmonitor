#include "spdlog/spdlog.h"
#include "argh.h"

#include "lsprobe_reader.h"
#include "lsprobe_event.h"
#include "fanotify_reader.h"
#include "fanotify_event.h"

#include "filter.h"

#include <signal.h>
#include <errno.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <atomic>

#include <stdio.h>
#include <iostream>

#include "stlab/concurrency/channel.hpp"
#include "stlab/concurrency/default_executor.hpp"
#include "stlab/concurrency/immediate_executor.hpp"

using namespace std::string_literals;

std::atomic_int lsp::FileEvent::count{};
std::atomic_int fan::FileEvent::count{};

void release_probe()
{
  spdlog::info("Tampering event reading...");
  char stop = '1';
  int fd = open("/sys/kernel/security/lsprobe/tamper", O_WRONLY);
  if (fd < 0 || write(fd, &stop, sizeof(char)) == -1)
  {
    int err = errno;
    fprintf(stderr, "Cannot write tamper byte to '/sys/kernel/security/lsprobe/tamper': %d - %s\n", err, ::strerror(err));
  }
  if (fd > 0)
    close(fd);
}

void signal_handler(int sig, siginfo_t *, void *)
{
  if (sig == SIGTERM || sig == SIGINT)
  {
    lsp::Reader::stopping.store(true);
    fan::Reader::stopping.store(true);
    release_probe();
  }
}

void setup_signal_handler()
{
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = signal_handler;
  if (sigaction(SIGINT, &sa, NULL) == -1
      ||sigaction(SIGTERM, &sa, NULL) == -1
      )
  {
    int err = errno;
    throw std::runtime_error(
	fmt::format("Unable to set signal handler: {0} - {1}", err, ::strerror(err))
	);
  }
}

void print_usage(const char * argv0)
{
  std::cerr << "Usage: "
    << argv0 << " [OPTIONS] [PREDICATE_OPTIONS]\n"
    << "\n"
    << "Options:\n"
    << "\t-d, --debug ................ Enable debug messages\n"
    << "\t-h, --help ................. This message\n"
    << "\t--fanotify ................. Use fanotify(7) instead of lsprobe\n"
    << "\n"
    << "Predicate options:\n"
    << "\t--file_contains=SUBSTRING .. Only requests to a file containing SUBSTRING\n"
    << "\t--file=PATH ................ Only requests to a file with the absolute PATH\n"
    << "\t--pid=ID ................... Only requests from a process with the specified ID\n"
    << "\t--uid=ID ................... Only requests from a process ran from the user with ID\n"
    << "\t--gid=ID ................... Only requests from a process ran from the group with ID\n"
    << "\t--process=PATH ............. Only requests from processes with the specified PATH (or NAME for fanotify)\n"
    << "\t--user=NAME ................ Only requests from processes ran from the user with the user NAME\n"
    << "\t--group=NAME ............... Only requests from processes ran from the group with with the group NAME\n"
    << std::endl;
}

struct FileEventPredicate
{
  argh::parser _cmdl{};
  pid_t _pid = static_cast<pid_t>(-1);
  uid_t _uid = static_cast<uid_t>(-1);
  gid_t _gid = static_cast<gid_t>(-1);

  FileEventPredicate(const argh::parser& cmdl)
    : _cmdl(cmdl)
  {
    readIds();
  }

  FileEventPredicate(argh::parser&& cmdl)
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

void listen_lsprobe_only(const FileEventPredicate& predicate)
{
  using event_t = std::unique_ptr<lsp::FileEvent>;
  stlab::sender<event_t> sender;
  stlab::receiver<event_t> receiver;
  std::tie(sender, receiver) = stlab::channel<event_t>(stlab::default_executor);

  std::atomic_bool done{false};

  auto r = receiver
    | [](event_t event)
      {
	spdlog::debug("lsp: {0} : pid[{1}] : cred [{2}({3}):{4}({5})] : op[{6}] : {7}"
	    , event->process.c_str()
	    , event->pcred.tgid
	    , event->user
	    , event->pcred.uid
	    , event->group
	    , event->pcred.gid
	    , static_cast<int>(event->code)
	    , event->filename.c_str()
	    );
	return event;
      }
    | lsp::filter<event_t, FileEventPredicate>{predicate} // TODO: multievent predicate
    | [](event_t event)
      {
	spdlog::info("lsp: {0} : pid[{1}] : cred [{2}({3}):{4}({5})] : op[{6}] : {7}"
	    , event->process.c_str()
	    , event->pcred.tgid
	    , event->user
	    , event->pcred.uid
	    , event->group
	    , event->pcred.gid
	    , static_cast<int>(event->code)
	    , event->filename.c_str()
	    );
      }; // last one shouldn't return


  receiver.set_ready();

  lsp::Reader lsreader(std::move(sender)); // TODO: copy, just for gigs
  lsreader.operator()(); // i.e. send()

  //TODO: multisender merging into 1 receiver
}

void listen_fanotify_only(const FileEventPredicate& predicate)
{
  using event_t = std::unique_ptr<fan::FileEvent>;
  stlab::sender<event_t> sender;
  stlab::receiver<event_t> receiver;
  std::tie(sender, receiver) = stlab::channel<event_t>(stlab::default_executor);

  std::atomic_bool done{false};

  auto r = receiver
    | [](event_t event)
      {
	spdlog::debug("fan: {0} : pid[{1}] : cred [{2}:{3}] : op[{4}] : {5}"
	    , event->process.c_str()
	    , event->pid
	    , event->uid
	    , event->gid
	    , static_cast<int>(event->code)
	    , event->filename.c_str()
	    );
	return event;
      }
//    | lsp::make_filter([](const event_t& event){return (event->filename == "fancy_name");})
//    | lsp::make_filter([](const event_t& event){return (event->filename.find("fancy_name") != std::string::npos);})
//    | lsp::make_filter([](const event_t& event){return (event->process.find("bash") != std::string::npos);})
//    | lsp::make_filter(predicate) // FIXME: what is worse: specify EventT explicitly or have several FilterPredicate types?
    | lsp::filter<event_t, FileEventPredicate>{predicate} // TODO: multievent predicate
    | [](event_t event)
      {
	spdlog::info("fan: {0} : pid[{1}] : cred [{2}:{3}] : op[{4}] : {5}"
	    , event->process.c_str()
	    , event->pid
	    , event->uid
	    , event->gid
	    , static_cast<int>(event->code)
	    , event->filename.c_str()
	    );
      }; // last one shouldn't return

  receiver.set_ready();

  fan::Reader reader("/home/"s, std::move(sender));
  reader.operator()(); // i.e. listen and send
}

int main(int argc, char ** argv)
{
  argh::parser cmdl(argc, argv);

  if (cmdl["-h"] || cmdl["--help"])
  {
    print_usage(argv[0]);
    return 0;
  }

  if (cmdl["-d"] || cmdl["--debug"])
  {
    spdlog::info("Debug mode enabled");
    spdlog::set_level(spdlog::level::debug);
  }
  else
    spdlog::set_level(spdlog::level::info);

  setup_signal_handler();


  if (cmdl["--fanotify"])
  {
    FileEventPredicate predicate(cmdl);
    spdlog::info("Starting fanotify listening...");
    std::thread fa_thread(listen_fanotify_only, predicate);
    fa_thread.join();
  }
  else
  {
    FileEventPredicate predicate(cmdl);
    spdlog::info("Starting lsprobe listening...");
    std::thread lsp_thread(listen_lsprobe_only, predicate);
    lsp_thread.join();
  }

  spdlog::info("That's all, folks!");
  return 0;
}
