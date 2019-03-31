#pragma once

#include "file_event_predicate.h"

#include "fanotify_reader.h"
#include "lsprobe_reader.h"

struct SourceManager
{
  void only(lsp::Reader&&, lsp::CmdlSimplePredicate&&);
  void only(fan::Reader&&, lsp::CmdlSimplePredicate&&);
  void any(lsp::Reader&&, fan::Reader&&, lsp::CmdlSimplePredicate&&);
  void count_strings(lsp::Reader&&, fan::Reader&&, lsp::CmdlSimplePredicate&&);
  void intersection(lsp::Reader&&, fan::Reader&&, lsp::CmdlSimplePredicate&&);
  void difference(lsp::Reader&&, fan::Reader&&, lsp::CmdlSimplePredicate&&);
};
