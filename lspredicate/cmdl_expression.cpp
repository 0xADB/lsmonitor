#include "cmdl_expression.h"
#include <boost/spirit/home/x3.hpp>
#include "lspredicate/expression_def.hpp"
#include "lspredicate/printer.hpp"
#include "spdlog/spdlog.h"

// ----------------------------------------------------------------------------

lsp::predicate::CmdlExpression::CmdlExpression(const lspredicate::ast::expression& expr)
  : _expr(expr)
{}

lsp::predicate::CmdlExpression::CmdlExpression(lspredicate::ast::expression&& expr)
  : _expr(std::move(expr))
{}

lsp::predicate::CmdlExpression::CmdlExpression(const std::string& expr)
{
  namespace x3 = boost::spirit::x3;

  if (!expr.empty())
  {
    auto it = expr.begin();
    auto end = expr.end();

    using iterator_type = std::string::const_iterator;
    using position_cache = boost::spirit::x3::position_cache<std::vector<iterator_type>>;
    position_cache positions{expr.begin(), expr.end()};

    auto const parser =
      x3::with<lspredicate::parser::position_cache_tag>(std::ref(positions))[lspredicate::parser::expression_def];

    bool r = x3::phrase_parse(it, end, parser, x3::ascii::space, _expr);

    if (r && it == end)
    {
      _empty = false;
      std::ostringstream out;
      lspredicate::ast::print(out, _expr);
      spdlog::debug("Predicate: {0}", out.str());
    }
    else
    {
      throw std::runtime_error(
	  fmt::format("Failed to parse predicate at: \"{0}\"", std::string(it, end))
	  );
    }
  }
}

// ----------------------------------------------------------------------------
