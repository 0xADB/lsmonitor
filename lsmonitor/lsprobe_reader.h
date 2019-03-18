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
    Reader() = default;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&&) = default;
    Reader& operator=(Reader&&) = default;
    ~Reader();

    Reader(stlab::sender<const lsp_event_t *>&& send)
      : _send(std::move(send))
    {}

    void operator()();

    int _eventsFd{};
    std::unique_ptr<std::byte[]> _buffer = std::make_unique<std::byte[]>(LSP_EVENT_MAX_SIZE);
    stlab::sender<const lsp_event_t *> _send;
    static std::atomic_bool stopping;
  };
} // lsp
