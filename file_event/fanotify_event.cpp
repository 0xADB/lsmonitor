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
	bool result = true;
	switch(ast.identifier)
	{
	  case lspredicate::ast::comparison_identifier::EVENT:
	    result = (static_cast<long>(_event->code) == boost::get<long>(ast.operation_.operand_));
	    break;
	  case lspredicate::ast::comparison_identifier::FILE_PATH:
	    result = (_event->filename == boost::get<std::string>(ast.operation_.operand_));
	    break;
	  case lspredicate::ast::comparison_identifier::PROCESS_PATH:
	    result = (_event->process == boost::get<std::string>(ast.operation_.operand_));
	    break;
	  case lspredicate::ast::comparison_identifier::PROCESS_PID:
	    result = (_event->pid == boost::get<long>(ast.operation_.operand_));
	    break;
	  case lspredicate::ast::comparison_identifier::PROCESS_UID:
	    result = (_event->uid == boost::get<long>(ast.operation_.operand_));
	    break;
	  case lspredicate::ast::comparison_identifier::PROCESS_GID:
	    result = (_event->gid == boost::get<long>(ast.operation_.operand_));
	    break;
	  default:
	     throw std::runtime_error(fmt::format("Unknown identifier index: {0}", static_cast<long>(ast.identifier)));
	}
	if (ast.operation_.operator_ == lspredicate::ast::comparison_operator::NEQ)
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
