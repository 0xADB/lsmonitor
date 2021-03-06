#include "printer.hpp"

#include <boost/foreach.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/assert.hpp>
#include <iostream>
#include <iomanip>
#include <string>

#include "fmt/format.h"

namespace lspredicate
{
  namespace ast
  {
    namespace
    {
      struct printer
      {
	typedef void result_type;

	printer(std::ostream& out)
	  : out(out)
	{}

	void operator()(bool ast) const
	{
	  out << std::boolalpha << ast;
	}

	void operator()(std::string const& ast) const
	{
	  out << "'" << ast.c_str() << "'";
	}

	void operator()(long ast) const
	{
	  out << ast;
	}

	void operator()(ast::disjunctive_operation const& ast) const
	{
	  out << " || ";
	  boost::apply_visitor(*this, ast.operand_);
	}

	void operator()(ast::conjunctive_operation const& ast) const
	{
	  out << " && ";
	  boost::apply_visitor(*this, ast.operand_);
	}

	void operator()(ast::comparison const& ast) const
	{
	  out << '(';
	  switch(ast.identifier)
	  {
	    case lspredicate::ast::comparison_identifier::EVENT       : out << "event"  ; break;
	    case lspredicate::ast::comparison_identifier::FILE_PATH   : out << "file"   ; break;
	    case lspredicate::ast::comparison_identifier::PROCESS_PATH: out << "process"; break;
	    case lspredicate::ast::comparison_identifier::PROCESS_PID : out << "pid"    ; break;
	    case lspredicate::ast::comparison_identifier::PROCESS_UID : out << "uid"    ; break;
	    case lspredicate::ast::comparison_identifier::PROCESS_GID : out << "gid"    ; break;
	    default:
	       throw std::runtime_error(fmt::format("Unknown identifier index: {0}", static_cast<int>(ast.identifier)));
	  }

	  if (ast.operation_.operator_ == ast::comparison_operator::NEQ)
	    out << " != ";
	  else if (ast.operation_.operator_ == ast::comparison_operator::EQ)
	    out << " == ";
	  else
	  {
	    BOOST_ASSERT(0);
	    return;
	  }
	  boost::apply_visitor(*this, ast.operation_.operand_);
	  out << ')';
	}

	void operator()(ast::negated const& ast) const
	{
	  if (ast.sign == '!')
	    out << '!';
	  boost::apply_visitor(*this, ast.operand_);
	}

	void operator()(ast::conjunctive_expression const& ast) const
	{
	  if (ast.tail.size())
	    out << '(';
	  boost::apply_visitor(*this, ast.head);
	  for (auto const& oper : ast.tail)
	    (*this)(oper);
	  if (ast.tail.size())
	    out << ')';
	}

	void operator()(ast::expression const& ast) const
	{
	  if (ast.tail.size())
	    out << '(';
	  boost::apply_visitor(*this, ast.head);
	  for (auto const& oper : ast.tail)
	    (*this)(oper);
	  if (ast.tail.size())
	    out << ')';
	}

	std::ostream& out;
      };
    }

    void print(std::ostream& out, ast::expression const& ast)
    {
        printer p(out);
        p(ast);
        out << std::endl;
    }
  } // ast
} // lspredicate
