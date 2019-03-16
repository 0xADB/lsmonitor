#include "lsprobe_reader.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <stdexcept>

lsp::Reader::~Reader()
{
  if (_eventsFd > 0)
    close(_eventsFd);
}

void lsp::Reader::operator()()
{
  _eventsFd = open("/sys/kernel/security/lsprobe/events", O_RDONLY);
  if (_eventsFd == -1)
  {
    int err = errno;
    throw std::runtime_error(
	fmt::format("Cannot open '/sys/kernel/security/lsprobe/events': {0} - {1}", err, ::strerror(err))
	);
  }

  lsp_event_t * event = new(_buffer.get()) lsp_event_t;

  ssize_t bytesRead = ::read(_eventsFd, event, LSP_EVENT_MAX_SIZE);
  while (bytesRead > 0)
  {
    spdlog::info("[{0:d}] [{1:d}] [{2:d}:{3:d}] {4}"
	, event->code
	, event->pid
	, event->uid
	, event->gid
	, lsp_event_field_first(event)->value
	);
    bytesRead = ::read(_eventsFd, event, LSP_EVENT_MAX_SIZE);
  }

  if (bytesRead < 0)
  {
    int err = errno;
    throw std::runtime_error(
	fmt::format("Events reading error: {0} - {1}", err, ::strerror(err))
	);
  }
}