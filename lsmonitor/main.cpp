#include "spdlog/spdlog.h"
#include "argh.h"

//#include "control_reader.h"
#include "lspredicate/cmdl_expression.h"
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
    ctl::broadcast::stopping.store(true);
    // ctl::Reader::stopping.store(true);
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
    << argv0 << " [OPTIONS] [MODES] [PREDICATE]\n"
    << "\n"
    << "Options:\n"
    << "\t-d, --debug .................... Enable debug messages\n"
    << "\t-h, --help ..................... This message\n"
    << "\t--fanotify ..................... Use fanotify(7) facility as a source (for testing purposes)\n"
    << "\t--lsprobe ...................... Use /sys/kernel/security/lsprobe/events as a source (default)\n"
    << "\n"
    << "Modes:\n"
    << "\t--only ......................... Use the only source (default)\n"
    << "\t--any .......................... Use all sources in parallel\n"
    << "\t--count_stringified ............ Use all sources and merge them stringified\n"
//     << "\t--intersection.................. Use all sources and show only events that came from all sources simultaneosly\n"
//    << "\t--difference ................... Use all sources and show only events that came from the only sources\n"
    << "\n"
    << "Predicate:\n"
    << "\t--expr='EXPRESSION' ............ Expression in a form:\n"
    << "\n"
    << "\t  (file==\"PATH\")||((pid!=ID)&&(process==\"PATH\"))\n"
    << "\n"
    << "\t  with available identifiers:\n"
    << "\t    file ....................... Full path of the file\n"
    << "\t    process .................... Full path of the process\n"
    << "\t    pid ........................ Process id\n"
    << "\t    uid ........................ User id\n"
    << "\t    gid ........................ Group id\n"
    << std::endl;
}

int main(int argc, char ** argv)
{
  argh::parser cmdl;

  cmdl.add_params({
      "file_contains"
      , "file"
      , "pid"
      , "uid"
      , "gid"
      , "process"
      , "expr"
      , "buffer"
      });
  cmdl.parse(argc, argv);

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

  if (cmdl["--any"])
  {
    spdlog::info("Starting in 'any' mode...");
    manager.any(lsp::Reader{}, fan::Reader{}, lsp::predicate::CmdlExpression(cmdl("--expr").str()));
  }
  else if (cmdl["--count_stringified"])
  {
    spdlog::info("Starting in 'count_stringified' mode...");
    manager.count_stringified(lsp::Reader{}, fan::Reader{}, lsp::predicate::CmdlExpression(cmdl("--expr").str()));
  }
//   else if (cmdl["--intersection"])
//   {
//     spdlog::info("Starting in 'intersection' mode...");
//     manager.intersection(lsp::Reader{}, fan::Reader{}, lsp::predicate::CmdlExpression(cmdl("--expr").str()));
//   }
//   else if (cmdl["--difference"])
//   {
//     spdlog::info("Starting in 'difference' mode...");
//     manager.difference(lsp::Reader{}, fan::Reader{}, lsp::predicate::CmdlExpression(cmdl("--expr").str()));
//   }
//   else if (cmdl["--buffered_difference"])
//   {
//     spdlog::info("Starting in 'buffered_difference' mode...");
//     size_t buffer_size = 3;
//     cmdl("--buffer", 3) >> buffer_size;
//     manager.buffered_difference(lsp::Reader{}, fan::Reader{}, lsp::predicate::CmdlExpression(cmdl("--expr").str()), buffer_size);
//   }
  else if (cmdl["--fanotify"])
  {
    spdlog::info("Starting fanotify listening...");
    manager.only(fan::Reader{}, lsp::predicate::CmdlExpression(cmdl("--expr").str()));
  }
  else
  {
    spdlog::info("Starting lsprobe listening...");
    manager.only(lsp::Reader{}, lsp::predicate::CmdlExpression(cmdl("--expr").str()));
  }

  return 0;
}
