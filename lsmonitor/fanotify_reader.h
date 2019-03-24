#pragma once

#include "fanotify_event.h"

#include <memory>
#include <cstddef>
#include <atomic>
#include "stlab/concurrency/channel.hpp"

#include <sys/types.h>
#include <sys/fanotify.h>

namespace fan
{
  struct Reader
  {
    using event_t = std::unique_ptr<FileEvent>;
    Reader() = default;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&&) = default;
    Reader& operator=(Reader&&) = default;
    ~Reader();

    Reader(const std::string& path, stlab::sender<event_t>&& send)
      : _path(path), _send(std::move(send))
    {}

    void handleEvents(int fad);
    void pollEvents(int fad);

    void operator()();

    int _fad{};
    std::string _path;
    stlab::sender<event_t> _send;

    static std::atomic_bool stopping;
  };
} // lsp
