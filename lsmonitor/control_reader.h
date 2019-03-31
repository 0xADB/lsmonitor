#pragma once

#include "control_event.h"
#include <experimental/filesystem>
#include <atomic>
#include <memory>

#include "stlab/concurrency/channel.hpp"

namespace ctl
{
  struct Reader
  {
    using event_t = std::unique_ptr<ControlEvent>;
    Reader() = default;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&&) = default;
    Reader& operator=(Reader&&) = default;
    ~Reader();

    void openTheWormhole();
    void handleEvents(int dataFd);

    void operator()(stlab::sender<event_t>&& send);

    int _fd{};
    std::experimental::filesystem::path _path = "/var/run/lsmonitor/ctl";
    stlab::sender<event_t> _send;

    static std::atomic_bool stopping;
  };
}
