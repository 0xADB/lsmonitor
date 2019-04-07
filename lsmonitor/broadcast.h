#pragma once

#include <atomic>
#include <memory>
#include <list>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "stlab/concurrency/channel.hpp"
#include "spdlog/spdlog.h"

#include <system_error>

namespace ctl
{
  struct broadcast
  {
    int _acceptFd{};
    std::vector<int> _dataFds{};
    std::string _value{};

    void send(std::string&& value);

    ~broadcast();

    void setup();
    void listen();

    static std::atomic_bool stopping;
  };
}
