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

    void operator()(stlab::sender<event_t>&& send);

    int _fd{};

    static std::atomic_bool stopping;
  };
} // lsp
