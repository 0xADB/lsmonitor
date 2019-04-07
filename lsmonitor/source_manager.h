#pragma once

#include "lspredicate/cmdl_expression.h"

#include "file_event/fanotify_reader.h"
#include "file_event/lsprobe_reader.h"

  struct SourceManager
  {
    template<typename Predicate> void only(lsp::Reader&&, Predicate&&);
    template<typename Predicate> void only(fan::Reader&&, Predicate&&);
    template<typename Predicate> void any(lsp::Reader&&, fan::Reader&&, Predicate&&);
    template<typename Predicate> void count_stringified(lsp::Reader&&, fan::Reader&&, Predicate&&);
    template<typename Predicate> void intersection(lsp::Reader&&, fan::Reader&&, Predicate&&);
    template<typename Predicate> void difference(lsp::Reader&&, fan::Reader&&, Predicate&&);
    template<typename Predicate> void buffered_difference(lsp::Reader&&, fan::Reader&&, Predicate&&, size_t buffer_size);
  };

#include "source_manager.hpp"
