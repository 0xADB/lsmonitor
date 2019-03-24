#pragma once

#include "lsp_event.h"
#include "lsprobe_event.h"

#include <memory>
#include <cstddef>
#include <atomic>
#include "stlab/concurrency/channel.hpp"

namespace lsp
{
  struct Reader
  {
    using event_t = std::unique_ptr<lsp::FileEvent>;

    Reader() = default;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&&) = default;
    Reader& operator=(Reader&&) = default;
    ~Reader();

    Reader(stlab::sender<event_t>&& send)
      : _send(std::move(send))
    {}

    void operator()();

    int _fd{};
    stlab::sender<event_t> _send;

    static std::atomic_bool stopping;
  };
} // lsp
