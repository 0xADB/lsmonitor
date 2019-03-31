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

std::atomic_bool lsp::Reader::stopping{};

lsp::Reader::~Reader()
{
  if (_fd > 0)
    close(_fd);
}

void lsp::Reader::operator()(stlab::sender<event_t>&& _send)
{
  _fd = open("/sys/kernel/security/lsprobe/events", O_RDONLY);
  if (_fd == -1)
  {
    std::error_code err(errno, std::system_category());
    throw std::runtime_error(
	fmt::format("Cannot open '/sys/kernel/security/lsprobe/events': {0} - {1}", err.value(), err.message())
	);
  }

  std::vector<std::byte> _buffer(LSP_EVENT_MAX_SIZE);
  lsp_event_t * event = new(_buffer.data()) lsp_event_t;

  ssize_t bytesRead = ::read(_fd, event, LSP_EVENT_MAX_SIZE);
  while (!stopping.load() && bytesRead > 0)
  {
    _send(std::make_unique<FileEvent>(event));
    if (!stopping.load())
      bytesRead = ::read(_fd, event, LSP_EVENT_MAX_SIZE);
  }

  std::error_code err(errno, std::system_category());
  close(_fd);
  _fd = 0;

  if (bytesRead < 0)
  {
    throw std::runtime_error(
	fmt::format("Events reading error: {0} - {1}", err.value(), err.message())
	);
  }
  else
    spdlog::debug("{0}: I quit", __PRETTY_FUNCTION__);
}
