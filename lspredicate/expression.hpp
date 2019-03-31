#pragma once

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include "ast.hpp"

namespace lspredicate
{
  namespace parser
  {
    namespace x3 = boost::spirit::x3;
    struct expression_class;
    using expression_type = x3::rule<expression_class, ast::disjunctive_expression>;
    BOOST_SPIRIT_DECLARE(expression_type)
  } // parser

  parser::expression_type expression();
} // lspredicate
