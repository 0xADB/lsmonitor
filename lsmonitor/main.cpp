#include "spdlog/spdlog.h"
#include "argh.h"

#include "control_reader.h"
#include "file_event_predicate.h"
#include "source_manager.h"

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <atomic>

#include <stdio.h>
#include <iostream>
#include <system_error>

#include "stlab/concurrency/channel.hpp"
#include "stlab/concurrency/default_executor.hpp"
#include "stlab/concurrency/immediate_executor.hpp"

void release_probe()
{
  spdlog::info("Tampering event reading...");
  char stop = '1';
  int fd = ::open("/sys/kernel/security/lsprobe/tamper", O_WRONLY);
  if (fd < 0
      || (::write(fd, &stop, sizeof(char)) == -1)
      )
  {
    std::error_code err(errno, std::system_category());
    fprintf(stderr
	, "Cannot write tamper byte to '/sys/kernel/security/lsprobe/tamper': %d - %s\n"
	, err.value()
	, err.message().c_str()
	);
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
    ctl::Reader::stopping.store(true);
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
    std::error_code err(errno, std::system_category());
    throw std::runtime_error(
	fmt::format("Unable to set signal handler: {0} - {1}", err.value(), err.message())
	);
  }
}

void print_usage(const char * argv0)
{
  std::cerr << "Usage: "
    << argv0 << " [OPTIONS] [SOURCES] [MODES] [PREDICATE]\n"
    << "\n"
    << "Options:\n"
    << "\t-d, --debug ................ Enable debug messages\n"
    << "\t-h, --help ................. This message\n"
    << "\n"
    << "Sources:\n"
    << "\t--fanotify ................. Use fanotify(7) facility as a source (default)\n"
    << "\t--lsprobe .................. Use /sys/kernel/security/lsprobe/events as a source\n"
    << "\n"
    << "Modes:\n"
    << "\t--only ..................... Use the only source (default)\n"
    << "\t--any ...................... Use all sources in parallel\n"
    << "\t--count_strings ............ Use all sources and merge them stringified\n"
    << "\t--intersection.............. Use all sources and show only events that came from all sources simultaneosly\n"
    << "\n"
    << "Simple predicate options (combined with the AND operator):\n"
    << "\t--file_contains=SUBSTRING .. Only requests to a file containing SUBSTRING\n"
    << "\t--file=PATH ................ Only requests to a file with the absolute PATH\n"
    << "\t--pid=ID ................... Only requests from a process with the specified ID\n"
    << "\t--uid=ID ................... Only requests from a process ran from the user with ID\n"
    << "\t--gid=ID ................... Only requests from a process ran from the group with ID\n"
    << "\t--process=PATH ............. Only requests from processes with the specified PATH (or NAME for fanotify)\n"
//    << "\t--user=NAME ................ Only requests from processes ran from the user with the user NAME\n"
//    << "\t--group=NAME ............... Only requests from processes ran from the group with with the group NAME\n"
    << std::endl;
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
    spdlog::info("Debug mode enabled...");
    spdlog::set_level(spdlog::level::debug);
  }
  else
    spdlog::set_level(spdlog::level::info);

  setup_signal_handler();

  SourceManager manager;
  lsp::CmdlSimplePredicate predicate(cmdl);

  if (cmdl["--any"])
  {
    spdlog::info("Starting in 'any' mode...");
    manager.any(lsp::Reader{}, fan::Reader{}, std::move(predicate));
  }
  else if (cmdl["--count_strings"])
  {
    spdlog::info("Starting in 'count_strings' mode...");
    manager.count_strings(lsp::Reader{}, fan::Reader{}, std::move(predicate));
  }
  else if (cmdl["--intersection"])
  {
    spdlog::info("Starting in 'intersection' mode...");
    manager.intersection(lsp::Reader{}, fan::Reader{}, std::move(predicate));
  }
  else if (cmdl["--difference"])
  {
    spdlog::info("Starting in 'difference' mode...");
    manager.difference(lsp::Reader{}, fan::Reader{}, std::move(predicate));
  }
  else if (cmdl["--lsprobe"])
  {
    spdlog::info("Starting lsprobe listening...");
    manager.only(lsp::Reader{}, std::move(predicate));
  }
  else
  {
    spdlog::info("Starting fanotify listening...");
    manager.only(fan::Reader{}, std::move(predicate));
  }

  spdlog::info("That's all, folks!");
  return 0;
}
