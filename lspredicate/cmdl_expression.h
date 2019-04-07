#pragma once

#include "lspredicate/ast.hpp"

namespace lsp
{
  namespace predicate
  {
    template<typename T>
      bool evaluate(const T& value, const lspredicate::ast::expression&);

    struct CmdlExpression
    {
      lspredicate::ast::expression  _expr{};
      bool _empty{true};

      CmdlExpression(const std::string& str);
      CmdlExpression(const lspredicate::ast::expression& expr);
      CmdlExpression(lspredicate::ast::expression&& expr);

      void print() const;
      bool empty() const {return _empty;}

      template<typename T>
	bool operator()(const T& value) const
	{
	  return (_empty || evaluate(value, _expr));
	}
    };

  } // predicate
} // lsp
