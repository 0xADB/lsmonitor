#include "spdlog/spdlog.h"

#include "lsprobe_reader.h"
#include "lsprobe_event.h"

#include <signal.h>
#include <errno.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <atomic>

#include "stlab/concurrency/channel.hpp"
#include "stlab/concurrency/default_executor.hpp"
#include "stlab/concurrency/immediate_executor.hpp"

void signal_handler(int sig, siginfo_t *, void *)
{
  if (sig == SIGTERM || sig == SIGINT)
  {
    spdlog::info("Tampering event reading...");
    lsp::Reader::stopping.store(true);
    char stop = '1';
    int fd = open("/sys/kernel/security/tamper", O_WRONLY);
    if (fd < 0 || write(fd, &stop, sizeof(char)) == -1)
    {
      int err = errno;
      throw std::runtime_error(
	  fmt::format("Cannot write tamper byte to '/sys/kernel/security/tamper': {0} - {1}", err, ::strerror(err))
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
	fmt::format("Unable to set signal handler: {0} - {1}", err, ::strerror(err))
	);
  }
}

int main()
{
  setup_signal_handler();

  spdlog::info("Listening events...");

  stlab::sender<const lsp_event_t *> send;
  stlab::receiver<const lsp_event_t *> receive;
  std::tie(send, receive) = stlab::channel<const lsp_event_t *>(stlab::default_executor);

  std::atomic_bool done{false};

  auto r = receive
    | [](const lsp_event_t * event) { return std::make_unique<lsp::FileEvent>(event); }
    | [](std::unique_ptr<lsp::FileEvent> event)
      {
	spdlog::trace("{0}: sending [{1:d}] [{2:d}:{3}] [{4}({5:d}):{6:d}] {7}"
	    , __PRETTY_FUNCTION__
	    , event->code
	    , event->pid
	    , event->process.c_str()
	    , event->user.c_str()
	    , event->uid
	    , event->gid
	    , event->filename.c_str()
	    );
	return event;
      }
    | [&done](std::unique_ptr<lsp::FileEvent> x) { done.store(static_cast<bool>(x)); };

  receive.set_ready();

  lsp::Reader lsreader(std::move(send));
  lsreader.operator()(); // i.e. send()

  while (!done.load())
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

  return 0;
}
