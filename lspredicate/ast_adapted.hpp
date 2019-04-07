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
    lspredicate::ast::disjunctive_operation
    , (lspredicate::ast::operand, operand_)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::conjunctive_operation
    , (lspredicate::ast::operand, operand_)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::disjunctive_expression
    , (lspredicate::ast::operand, head)
    , (std::list<lspredicate::ast::disjunctive_operation>, tail)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::conjunctive_expression
    , (lspredicate::ast::operand, head)
    , (std::list<lspredicate::ast::conjunctive_operation>, tail)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::comparison_operation
    , (lspredicate::ast::comparison_operator, operator_)
    , (lspredicate::ast::comparison_value, operand_)
    )

BOOST_FUSION_ADAPT_STRUCT(
    lspredicate::ast::comparison
    , (lspredicate::ast::comparison_identifier, identifier)
    , (lspredicate::ast::comparison_operation, operation_)
    )
