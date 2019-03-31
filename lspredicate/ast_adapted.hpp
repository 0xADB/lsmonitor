#pragma once

#include <boost/fusion/include/adapt_struct.hpp>
#include "ast.hpp"
#include <string>

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::negated
    , (char, sign)
    , (lspredicate::ast::operand, operand_)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::operation
    , (std::string, operator_)
    , (lspredicate::ast::operand, operand_)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::disjunctive_expression
    , (lspredicate::ast::operand, head)
    , (std::list<lspredicate::ast::operation>, tail)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::conjunctive_expression
    , (lspredicate::ast::operand, head)
    , (std::list<lspredicate::ast::operation>, tail)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::comparison_operation
    , (std::string, operator_)
    , (lspredicate::ast::comparison_value, operand_)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::comparison
    , (std::string, identifier)
    , (lspredicate::ast::comparison_operation, operation_)
    )
