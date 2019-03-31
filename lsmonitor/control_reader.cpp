#include "control_reader.h"

#include <string>
#include <stdexcept>
#include <system_error>

#include "spdlog/spdlog.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <cstring>
#include <cstddef>

namespace fs = std::experimental::filesystem;

std::atomic_bool ctl::Reader::stopping{};

ctl::Reader::~Reader()
{
  if (_fd > 0)
  {
    close(_fd);
    std::error_code err{};
    fs::remove(_path, err);
  }
}

void ctl::Reader::openTheWormhole()
{
  std::error_code err{};
  if (fs::exists(_path, err))
    fs::remove(_path, err);
  else
    fs::create_directories(_path.remove_filename(), err);

  if (err)
    throw std::runtime_error(
	fmt::format("Unable to create '{0}': {1} - {2}", _path.remove_filename().native(), err.value(), err.message())
	);

  _fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (_fd == -1)
  {
    err.assign(errno, std::system_category());
    throw std::runtime_error(
	fmt::format("Unable to create an AF_UNIX socket for '{0}': {1} - {2}", _path.native(), err.value(), err.message())
	);
  }

  struct sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  auto copiedCount = _path.native().copy(addr.sun_path, sizeof(addr.sun_path) - 1);
  addr.sun_path[copiedCount] = 0;

  if (::bind(_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(struct sockaddr_un)) == -1)
  {
    err.assign(errno, std::system_category());
    close(_fd);
    _fd = 0;
    throw std::runtime_error(
	fmt::format("Unable to bind an AF_UNIX socket to '{0}': {1} - {2}", _path.native(), err.value(), err.message())
	);
  }
}

void ctl::Reader::handleEvents(int dataFd)
{
  struct ucred ucred{};
  std::error_code err{};

  socklen_t ucredSize = sizeof(struct ucred);
  if (::getsockopt(dataFd, SOL_SOCKET, SO_PEERCRED, &ucred, &ucredSize) == -1)
  {
    err.assign(errno, std::system_category());
    spdlog::warn(
	"Unable to identify the peer of '{0}': {1} - {2}", _path.native(), err.value(), err.message()
	);
  }
  else if (ucredSize != sizeof(struct ucred))
  {
    throw std::runtime_error(
	fmt::format("getsockopt()'s struct ucred size mismatch: {0} vs {1} expected", ucredSize, sizeof(struct ucred))
	);
  }

  std::vector<char> buffer(64);
  buffer[buffer.size() - 1] = 0;

  ssize_t bytes = ::read(dataFd, buffer.data(), buffer.size() - 2);
  if (bytes > 0)
  {
    buffer[bytes] = 0;
    spdlog::debug("read {0} bytes from the connection on '{1}': {2}", bytes, _path.native(), buffer.data());
    _send(std::make_unique<ControlEvent>(buffer.data(), bytes, ucred));
  }
  else if (bytes < 0)
  {
    err.assign(errno, std::system_category());
    spdlog::warn(
	"Unable to read from the connection an AF_UNIX socket to '{0}': {1} - {2}", _path.native(), err.value(), err.message()
	);
  }
}

void ctl::Reader::operator()(stlab::sender<event_t>&& send)
{
  openTheWormhole();

  std::error_code err{};
  if (::listen(_fd, 1) == -1)
  {
    err.assign(errno, std::system_category());
    close(_fd);
    _fd = 0;
    throw std::runtime_error(
	fmt::format("Unable to listen the AF_UNIX socket on '{0}': {1} - {2}", _path.native(), err.value(), err.message())
	);
  }

  _send = std::move(send);

  while(!stopping.load())
  {
    int dataFd = ::accept(_fd, NULL, NULL);
    if (dataFd != -1)
    {
      handleEvents(dataFd);
      close(dataFd);
    }
    else if (errno != EINTR)
    {
      err.assign(errno, std::system_category());
      close(_fd);
      _fd = 0;
      throw std::runtime_error(
	  fmt::format("Unable to accept a connection on '{0}': {1} - {2}", _path.native(), err.value(), err.message())
	  );
    }
  }
}
