#pragma once

#include "fanotify_event.h"
#include "lsprobe_event.h"
#include "argh.h"
#include "lspredicate/ast.hpp"

namespace lsp
{
  namespace predicate
  {
    struct CmdlSimpleConjunctive
    {
      using lsp_event_t = std::unique_ptr<lsp::FileEvent>;
      using fan_event_t = std::unique_ptr<fan::FileEvent>;

      argh::parser _cmdl{};
      pid_t _pid = static_cast<pid_t>(-1);
      uid_t _uid = static_cast<uid_t>(-1);
      gid_t _gid = static_cast<gid_t>(-1);

      CmdlSimpleConjunctive(const argh::parser& cmdl);
      CmdlSimpleConjunctive(argh::parser&& cmdl);

      void print() const;
      void readIds();

      bool operator()(const lsp_event_t& event) const;
      bool operator()(const fan_event_t& event) const;
    };

    struct CmdlExpression
    {
      using lsp_event_t = std::unique_ptr<lsp::FileEvent>;
      using fan_event_t = std::unique_ptr<fan::FileEvent>;

      lspredicate::ast::expression  _expr{};

      CmdlExpression(const std::string& str);
      CmdlExpression(const lspredicate::ast::expression& expr);
      CmdlExpression(lspredicate::ast::expression&& expr);

      void print() const;

      bool operator()(const lsp_event_t& event) const;
      bool operator()(const fan_event_t& event) const;
    };

  } // predicate
} // lsp
