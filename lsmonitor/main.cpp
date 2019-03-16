#include "spdlog/spdlog.h"

#include "lsprobe_reader.h"

#include <signal.h>
#include <errno.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

void signal_handler(int sig, siginfo_t *, void *)
{
  if (sig == SIGTERM || sig == SIGINT)
  {
    spdlog::info("Tampering event reading...");
    char stop = '1';
    int fd = open("/sys/kernel/security/tamper", O_WRONLY);
    if (fd < 0 || write(fd, &stop, sizeof(char)) == -1)
    {
      int err = errno;
      throw std::runtime_error(
	  fmt::format("Cannot write tamper byte to '/sys/kernel/security/tamper': {0} - {1}", err, strerror(err))
	  );
    }
    close(fd);
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
	fmt::format("Unable to set signal handler: {0} - {1}", err, strerror(err))
	);
  }
}

int main()
{
  setup_signal_handler();

  spdlog::info("Listening events...");

  lsp::Reader lsreader;
  lsreader.operator()();

  return 0;
}
