#include "broadcast.h"

#include "stlab/concurrency/channel.hpp"

#include <string>
#include <stdexcept>
#include <system_error>

#include "spdlog/spdlog.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <cstring>
#include <cstddef>

#include <system_error>

#include "spdlog/spdlog.h"

std::atomic_bool ctl::broadcast::stopping{};

ctl::broadcast::~broadcast()
{
  for (auto fd : _dataFds)
    ::close(fd);
  if (_acceptFd > 0)
    ::close(_acceptFd);
}

void ctl::broadcast::setup()
{
  _acceptFd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (_acceptFd == -1)
  {
    std::error_code err(errno, std::system_category());
    throw std::runtime_error(
	fmt::format("Unable to create an PF_INET socket: {0} - {1}", err.value(), err.message())
	);
  }

  struct sockaddr_in addr{};
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(50001);

  if (::bind(_acceptFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(struct sockaddr_in)) == -1)
  {
    std::error_code err(errno, std::system_category());
    throw std::runtime_error(
	fmt::format("Unable to bind an PF_INET socket: {0} - {1}", err.value(), err.message())
	);
  }

  if (::listen(_acceptFd, 1) == -1)
  {
    std::error_code err(errno, std::system_category());
    throw std::runtime_error(
	fmt::format("Unable to listen the PF_INET socket: {0} - {1}", err.value(), err.message())
	);
  }
}

void ctl::broadcast::listen()
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));

  while(!stopping.load())
  {
    socklen_t size = sizeof(struct sockaddr_in);
    int fd = ::accept(_acceptFd, reinterpret_cast<struct sockaddr *>(&addr), &size);
    if (fd != -1)
    {
      _dataFds.push_back(fd);
    }
    else if (errno == EINTR)
    {
      spdlog::debug("{0}: exiting", __PRETTY_FUNCTION__);
      break;
    }
    else
    {
      std::error_code err(errno, std::system_category());
      throw std::runtime_error(
	  fmt::format("Unable to accept a connection: {0} - {1}", err.value(), err.message())
	  );
    }
  }
}

void ctl::broadcast::send(std::string&& value)
{
  _value = std::move(value);
  spdlog::debug("{0}: sending {1}", __PRETTY_FUNCTION__, _value);
  for (auto fd : _dataFds)
  {
    if (::send(fd, _value.c_str(), _value.size() + 1, MSG_NOSIGNAL) == -1)
    {
      std::error_code err(errno, std::system_category());
      spdlog::warn("{0}: socket [{1}] error: {2} - {3}", __PRETTY_FUNCTION__, fd, err.value(), err.message());
      _dataFds.erase(std::remove(std::begin(_dataFds), std::end(_dataFds), fd), std::end(_dataFds));
      ::close(fd);
    }
  }
}

