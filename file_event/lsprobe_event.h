#pragma once

#include "lsp_event.h"
#include "utility.h"

#include "spdlog/spdlog.h"

#include <string>
#include <vector>
#include <memory>

#include <sys/types.h>
#include "lspredicate/ast.hpp"
#include "lspredicate/cmdl_expression.h"

namespace lsp
{
  struct FileEvent
  {
    FileEvent(const lsp_event_t * event)
      : code(static_cast<lsp_event_code_t>(event->code))
      , pcred(event->pcred)
      , filename(
	  lsp_event_field_first_const(event)->value
	  )
      , process(
	  lsp_event_field_get_const(event, 1)->value
	  )
    {}

    FileEvent() = default;
    FileEvent(FileEvent&&) = default;
    FileEvent(const FileEvent&) = default;
    FileEvent& operator=(FileEvent&&) = default;
    FileEvent& operator=(const FileEvent&) = default;
    ~FileEvent() = default;

    std::string stringify() const;

    lsp_event_code_t code{};
    lsp_cred_t pcred{};
    std::string filename{};
    std::string process{};
  };

  namespace predicate
  {
    template<>
    bool evaluate<std::unique_ptr<FileEvent>>(const std::unique_ptr<FileEvent>& event, lspredicate::ast::expression const& ast);
  }
}
