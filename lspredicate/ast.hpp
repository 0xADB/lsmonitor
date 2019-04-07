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

    enum class comparison_operator {EQ, NEQ};
    enum class comparison_identifier
    {
	EVENT
      , FILE_PATH
      , PROCESS_PATH
      , PROCESS_PID
      , PROCESS_UID
      , PROCESS_GID
    };

    struct negated;
    struct disjunctive_expression;
    struct conjunctive_expression;
    struct comparison;

    struct operand
      : x3::variant<
	bool
	, x3::forward_ast<negated>
	, x3::forward_ast<comparison>
	, x3::forward_ast<disjunctive_expression>
	, x3::forward_ast<conjunctive_expression>
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

    struct disjunctive_operation : x3::position_tagged
    {
      operand operand_;
    };

    struct conjunctive_operation : x3::position_tagged
    {
      operand operand_;
    };

    struct disjunctive_expression : x3::position_tagged
    {
      operand head;
      std::list<disjunctive_operation> tail;
    };

    struct conjunctive_expression : x3::position_tagged
    {
      operand head;
      std::list<conjunctive_operation> tail;
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
      comparison_operator operator_;
      comparison_value operand_;
    };

    struct comparison : x3::position_tagged
    {
      comparison_identifier identifier;
      comparison_operation operation_;
    };

    using boost::fusion::operator<<;

    using expression = disjunctive_expression;
  } //ast
} //lspredicate
