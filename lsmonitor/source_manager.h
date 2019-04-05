#pragma once

#include "file_event_predicate.h"

#include "fanotify_reader.h"
#include "lsprobe_reader.h"

  struct SourceManager
  {
    template<typename Predicate> void only(lsp::Reader&&, Predicate&&);
    template<typename Predicate> void only(fan::Reader&&, Predicate&&);
    template<typename Predicate> void any(lsp::Reader&&, fan::Reader&&, Predicate&&);
    template<typename Predicate> void count_strings(lsp::Reader&&, fan::Reader&&, Predicate&&);
    template<typename Predicate> void intersection(lsp::Reader&&, fan::Reader&&, Predicate&&);
    template<typename Predicate> void difference(lsp::Reader&&, fan::Reader&&, Predicate&&);
    template<typename Predicate> void buffered_difference(lsp::Reader&&, fan::Reader&&, Predicate&&, size_t buffer_size);
  };

#include "source_manager.hpp"
