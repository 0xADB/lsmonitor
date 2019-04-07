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
    using event_t = std::unique_ptr<fan::FileEvent>;

    Reader() = default;

    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    Reader(Reader&&) = default;
    Reader& operator=(Reader&&) = default;

    ~Reader();

    void handleEvents(int fad);
    void pollEvents(int fad);

    void operator()(stlab::sender<event_t>&& send, std::string&& path);

    int _fad{};
    stlab::sender<event_t> _send;

    static std::atomic_bool stopping;
  };

  // namespace v2
  // {
  //   struct Reader
  //   {
  //     using event_t = std::unique_ptr<FileEvent>;

  //     Reader() = default;
  //     Reader(const Reader&) = delete;
  //     Reader& operator=(const Reader&) = delete;
  //     Reader(Reader&&) = default;
  //     Reader& operator=(Reader&&) = default;
  //     ~Reader();

  //     void operator()(const std::string& path);

  //     void stop(){stopping.store(true);}

  //     auto& receiver(){return _channel.second;}

  //     int _fd{};

  //     static std::atomic_bool stopping;
  //   };
  // } // v2

} // lsp
