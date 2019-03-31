#pragma once

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <stdexcept>

#include "ast.hpp"
#include "ast_adapted.hpp"
#include "expression.hpp"

namespace lspredicate
{
  namespace parser
  {
    namespace x3 = boost::spirit::x3;

    struct position_cache_tag;

    struct annotate_position
    {
      template<typename T, typename It, typename Context>
	inline void on_success(It const& first, It const& last, T& ast, Context const& context)
	{
	  auto& position_cache = x3::get<position_cache_tag>(context).get();
	  position_cache.annotate(ast, first, last);
	}
    };

    // struct error_handler
    // {
    //   template<typename It, typename Exception, typename Context>
    //     x3::error_handler_result on_error(It& first, It const& last, Exception const& x, Context const& context)
    //     {
    //       auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
    //       std::string message = "Error: expecting: " + x.which() + " here: ";
    //       error_handler(x.where(), message);
    //       return x3::error_handler_result::fail;
    //     }
    // };

    struct additive_expr_class;
    struct multiplicative_expr_class;
    struct unary_expr_class;
    struct primary_expr_class;
    struct comparison_expr_class;
    struct comparison_operation_class;
    struct comparison_value_class;

    using additive_expr_type         = x3::rule<additive_expr_class        , ast::expression>;
    using multiplicative_expr_type   = x3::rule<multiplicative_expr_class  , ast::expression>;
    using unary_expr_type            = x3::rule<unary_expr_class           , ast::operand>;
    using primary_expr_type          = x3::rule<primary_expr_class         , ast::operand>;
    using comparison_expr_type       = x3::rule<comparison_expr_class      , ast::comparison>;
    using comparison_operation_type  = x3::rule<comparison_operation_class , ast::comparison_operation>;
    using comparison_value_type      = x3::rule<comparison_value_class     , ast::comparison_value>;

    additive_expr_type         const additive_expr         = "additive_expr";
    multiplicative_expr_type   const multiplicative_expr   = "multiplicative_expr";
    unary_expr_type            const unary_expr            = "unary_expr";
    primary_expr_type          const primary_expr          = "primary_expr";
    comparison_expr_type       const comparison_expr       = "comparison_expr";
    comparison_operation_type  const comparison_operation  = "comparison_operation";
    comparison_value_type      const comparison_value      = "comparison_value";

    expression_type const expression = "expression";

    auto const additive_expr_def =
      multiplicative_expr
      >> *((+(x3::char_('|')) >> x3::expect[multiplicative_expr]));

    auto const multiplicative_expr_def =
      unary_expr
      >> *((+(x3::char_('&')) >> x3::expect[unary_expr]));

    auto const unary_expr_def =
      primary_expr
      | (x3::char_('!') >> x3::expect[primary_expr]);

    auto const primary_expr_def =
      x3::bool_
      | ('(' >> comparison_expr >> ')')
      | ('(' >> expression >> ')');

    auto const comparison_expr_def =
      (+(x3::alpha)) >> x3::expect[comparison_operation];

    auto const comparison_operation_def =
      (x3::string("==") >> comparison_value)
      | (x3::string("!=") >> comparison_value);

    auto const quoted_string =
      x3::lexeme['"' >> +(x3::char_ - '"') >> '"']
      | x3::lexeme['\'' >> +(x3::char_ - '\'') >> '\''];

    auto const comparison_value_def =
      x3::bool_
      | x3::long_
      | quoted_string;

    auto const expression_def = additive_expr;

    BOOST_SPIRIT_DEFINE(
	expression
	, additive_expr
	, multiplicative_expr
	, unary_expr
	, primary_expr
	, comparison_expr
	, comparison_operation
	, comparison_value
	)

    struct unary_expr_class : annotate_position {};
    struct primary_expr_class : annotate_position {};
    struct comparison_expr_class : annotate_position {};
    struct comparison_operation_class : annotate_position {};
    struct comparison_value_class : annotate_position {};
    struct expression_class : annotate_position/*, error_handler*/ {};
  } // parser

  parser::expression_type expression()
  {
    return parser::expression;
  }

  
} //lspredicate
