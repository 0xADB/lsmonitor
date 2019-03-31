#pragma once

#include <ostream>
#include "ast.hpp"

namespace lspredicate
{
  namespace ast
  {
    void print(std::ostream& out, ast::expression const& ast);
  }
}
