#pragma once

#include "file_event_predicate.h"

#include "fanotify_reader.h"
#include "lsprobe_reader.h"

struct SourceManager
{
  void only(lsp::Reader&&, lsp::predicate::CmdlSimpleConjunctive&&);
  void only(fan::Reader&&, lsp::predicate::CmdlSimpleConjunctive&&);
  void any(lsp::Reader&&, fan::Reader&&, lsp::predicate::CmdlSimpleConjunctive&&);
  void count_strings(lsp::Reader&&, fan::Reader&&, lsp::predicate::CmdlSimpleConjunctive&&);
  void intersection(lsp::Reader&&, fan::Reader&&, lsp::predicate::CmdlSimpleConjunctive&&);
  void difference(lsp::Reader&&, fan::Reader&&, lsp::predicate::CmdlSimpleConjunctive&&);
};
