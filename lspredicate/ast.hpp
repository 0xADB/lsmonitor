#pragma once

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/fusion/include/io.hpp>
#include <list>
#include <string>

namespace lspredicate
{
  namespace ast
  {
    namespace x3 = boost::spirit::x3;

    struct negated;
    struct expression;
    struct comparison;

    struct operand
      : x3::variant<
	bool
	, x3::forward_ast<negated>
	, x3::forward_ast<comparison>
	, x3::forward_ast<expression>
	>
    {
      using base_type::base_type;
      using base_type::operator=;
    };

    struct negated
    {
      char sign;
      operand operand_;
    };

    struct operation : x3::position_tagged
    {
      std::string operator_;
      operand operand_;
    };

    struct expression : x3::position_tagged
    {
      operand head;
      std::list<operation> tail; // TODO: vector
    };

    struct comparison_value
      : x3::variant<
        bool
        , long
        , std::string
        >
    {
      using base_type::base_type;
      using base_type::operator=;
    };

    struct comparison_operation : x3::position_tagged
    {
      std::string operator_;
      comparison_value operand_;
    };

    struct comparison : x3::position_tagged
    {
      std::string identifier;
      comparison_operation operation_;
    };

    using boost::fusion::operator<<;
  } //ast
} //lspredicate
