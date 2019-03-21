#pragma once

#include "event.h"
#include "file_event.h"

#include <memory>
#include <cstddef>
#include <atomic>
#include "stlab/concurrency/channel.hpp"

#include <sys/types.h>
#include <sys/fanotify.h>

namespace fanotify
{
  struct Reader
  {
    Reader() = default;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&&) = default;
    Reader& operator=(Reader&&) = default;
    ~Reader();

    Reader(stlab::sender<const lsp::Event *>&& send)
      : _send(std::move(send))
    {}

    void operator()();

    int _fanFd{};
    int _pollNum{};
    std::unique_ptr<struct fanotify_event_metadata[]> _buffer = std::make_unique<struct fanotify_event_metadata[]>(128);
    stlab::sender<const lsp::Event *> _send;
    static std::atomic_bool stopping;
  };
} // lsp
