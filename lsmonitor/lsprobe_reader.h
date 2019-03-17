#pragma once

#include "lsp_event.h"

#include <memory>
#include <cstddef>
#include <atomic>

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

    void operator()();

    int _eventsFd{};
    std::unique_ptr<std::byte[]> _buffer = std::make_unique<std::byte[]>(LSP_EVENT_MAX_SIZE);
    static std::atomic_bool stopping;
  };
} // lsp
