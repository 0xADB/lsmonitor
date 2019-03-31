#include "fanotify_event.h"

#include <boost/variant/apply_visitor.hpp>
#include <boost/assert.hpp>

namespace fan
{
  namespace predicate
  {
    struct CmdlExpressionEvaluator
    {
      using event_t = std::unique_ptr<FileEvent>;
      using result_type = bool;

      const event_t& _event;

      CmdlExpressionEvaluator(const event_t& event)
	: _event(event)
      {}

      bool operator()(bool ast) const
      {
	return ast;
      }

      // const std::string& operator()(std::string const& ast) const
      // {
      //   return ast;
      // }

      // long operator()(long ast) const
      // {
      //   return ast;
      // }

      bool operator()(lspredicate::ast::comparison const& ast) const
      {
	// TODO: hash names?
	bool result = false;
	if (ast.identifier == "file")
	{
	  result = (_event->filename == boost::get<std::string>(ast.operation_.operand_));
	}
	else if (ast.identifier == "process")
	{
	  result = (_event->process == boost::get<std::string>(ast.operation_.operand_));
	}
	if (ast.operation_.operator_.front() == '!')
	  result = !result;
	return result;
      }

      bool operator()(lspredicate::ast::negated const& ast) const
      {
	bool r = boost::apply_visitor(*this, ast.operand_);
	return (ast.sign == '!' ? !r : r);
      }

      bool operator()(lspredicate::ast::disjunctive_expression const& ast) const
      {
	bool result = boost::apply_visitor(*this, ast.head);
	if (!result && ast.tail.size())
	{
	  for (auto it = std::begin(ast.tail)
	      , endIt = std::end(ast.tail)
	      ; (it != endIt) && !result
	      ; ++it
	      )
	  {
	    result = (result || boost::apply_visitor(*this, it->operand_));
	  }
	}
	return result;
      }

      bool operator()(lspredicate::ast::conjunctive_expression const& ast) const
      {
	bool result = boost::apply_visitor(*this, ast.head);
	if (result && ast.tail.size())
	{
	  for (auto it = std::begin(ast.tail)
	      , endIt = std::end(ast.tail)
	      ; (it != endIt) && result
	      ; ++it
	      )
	  {
	    result = (result && boost::apply_visitor(*this, it->operand_));
	  }
	}
	return result;
      }

    };

    bool evaluate(const std::unique_ptr<fan::FileEvent>& event, lspredicate::ast::expression const& ast)
    {
      return CmdlExpressionEvaluator{event}(ast);
    }
  } // predicate
} // fan
