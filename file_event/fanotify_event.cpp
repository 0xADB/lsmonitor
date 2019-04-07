#include "fanotify_event.h"

#include <boost/variant/apply_visitor.hpp>
#include <boost/assert.hpp>

#include "fmt/format.h"

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

      bool operator()(lspredicate::ast::comparison const& ast) const
      {
	// TODO: hash names?
	bool result = true;
	bool matched = false;
	if (ast.identifier == "file")
	{
	  matched = true;
	  result = (_event->filename == boost::get<std::string>(ast.operation_.operand_));
	}
	else if (ast.identifier == "process")
	{
	  matched = true;
	  result = (_event->process == boost::get<std::string>(ast.operation_.operand_));
	}
	else if (ast.identifier == "pid")
	{
	  matched = true;
	  result = (_event->pid == boost::get<long>(ast.operation_.operand_));
	}
	else if (ast.identifier == "uid")
	{
	  matched = true;
	  result = (_event->uid == boost::get<long>(ast.operation_.operand_));
	}
	else if (ast.identifier == "gid")
	{
	  matched = true;
	  result = (_event->uid == boost::get<long>(ast.operation_.operand_));
	}
	if (matched && ast.operation_.operator_.front() == '!')
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

  } // predicate

  std::string FileEvent::stringify() const
  {
    return fmt::format("lsp: {0} : pid[{1}] : uid[{2}] : gid[{3}] : op[{4}] : {5}"
	    , process
	    , pid
	    , uid
	    , gid
	    , static_cast<int>(code)
	    , filename
	    );
  }


} // fan

namespace lsp
{
  namespace predicate
  {
    template<>
    bool evaluate<std::unique_ptr<fan::FileEvent>>(const std::unique_ptr<fan::FileEvent>& event, lspredicate::ast::expression const& ast)
    {
      return fan::predicate::CmdlExpressionEvaluator{event}(ast);
    }
  }
}
