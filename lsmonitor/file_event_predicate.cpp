#include "file_event_predicate.h"
#include <boost/spirit/home/x3.hpp>
#include "lspredicate/expression_def.hpp"
#include "lspredicate/printer.hpp"

// ----------------------------------------------------------------------------

lsp::predicate::CmdlSimpleConjunctive::CmdlSimpleConjunctive(const argh::parser& cmdl)
  : _cmdl(cmdl)
{
  readIds();
}

lsp::predicate::CmdlSimpleConjunctive::CmdlSimpleConjunctive(argh::parser&& cmdl)
  : _cmdl(std::move(cmdl))
{
  readIds();
}

void lsp::predicate::CmdlSimpleConjunctive::print() const
{
  if (_pid != static_cast<pid_t>(-1)) spdlog::debug("Found cmdl[\"pid\"]: {0}", _pid);
  if (_uid != static_cast<uid_t>(-1)) spdlog::debug("Found cmdl[\"uid\"]: {0}", _uid);
  if (_gid != static_cast<gid_t>(-1)) spdlog::debug("Found cmdl[\"gid\"]: {0}", _gid);
  if (_cmdl("file"         )) spdlog::debug("Found cmdl[\"file\"]: {0}", _cmdl("file").str());
  if (_cmdl("user"         )) spdlog::debug("Found cmdl[\"user\"]: {0}", _cmdl("user").str());
  if (_cmdl("group"        )) spdlog::debug("Found cmdl[\"group\"]: {0}", _cmdl("group").str());
  if (_cmdl("process"      )) spdlog::debug("Found cmdl[\"process\"]: {0}", _cmdl("process").str());
  if (_cmdl("file_contains")) spdlog::debug("Found cmdl[\"file_contains\"]: {0}", _cmdl("file_contains").str());
}

void lsp::predicate::CmdlSimpleConjunctive::readIds()
{
  _cmdl("pid", -1) >> _pid;
  _cmdl("uid", -1) >> _uid;
  _cmdl("gid", -1) >> _gid;
  print();
}

bool lsp::predicate::CmdlSimpleConjunctive::operator()(const lsp_event_t& event) const
{
  return (
         (_pid == static_cast<pid_t>(-1) ? true : (event->pcred.tgid == _pid))
      && (_uid == static_cast<uid_t>(-1) ? true : (event->pcred.uid == _uid))
      && (_gid == static_cast<gid_t>(-1) ? true : (event->pcred.gid == _gid))
      && (!_cmdl("file"         )        ? true : (event->filename == _cmdl("file").str()))
      && (!_cmdl("user"         )        ? true : (event->user     == _cmdl("user").str()))
      && (!_cmdl("group"        )        ? true : (event->group    == _cmdl("group").str()))
      && (!_cmdl("process"      )        ? true : (event->process  == _cmdl("process").str()))
      && (!_cmdl("file_contains")        ? true : (event->filename.find(_cmdl("file_contains").str()) != std::string::npos))
      );
}

bool lsp::predicate::CmdlSimpleConjunctive::operator()(const fan_event_t& event) const
{
  return (
         (_pid == static_cast<pid_t>(-1) ? true : (event->pid == _pid))
      && (_uid == static_cast<uid_t>(-1) ? true : (event->uid == _uid))
      && (_gid == static_cast<gid_t>(-1) ? true : (event->gid == _gid))
      && (!_cmdl("file"         )        ? true : (event->filename == _cmdl("file").str()))
      && (!_cmdl("user"         )        ? true : (event->user     == _cmdl("user").str()))
      && (!_cmdl("group"        )        ? true : (event->group    == _cmdl("group").str()))
      && (!_cmdl("process"      )        ? true : (event->process  == _cmdl("process").str()))
      && (!_cmdl("file_contains")        ? true : (event->filename.find(_cmdl("file_contains").str()) != std::string::npos))
      );
}

// ----------------------------------------------------------------------------

lsp::predicate::CmdlExpression::CmdlExpression(const lspredicate::ast::expression& expr)
  : _expr(expr)
{}

lsp::predicate::CmdlExpression::CmdlExpression(lspredicate::ast::expression&& expr)
  : _expr(std::move(expr))
{}

lsp::predicate::CmdlExpression::CmdlExpression(const std::string& expr)
{
  namespace x3 = boost::spirit::x3;

  auto it = expr.begin();
  auto end = expr.end();

  using iterator_type = std::string::const_iterator;
  using position_cache = boost::spirit::x3::position_cache<std::vector<iterator_type>>;
  position_cache positions{expr.begin(), expr.end()};

  auto const parser =
    x3::with<lspredicate::parser::position_cache_tag>(std::ref(positions))[lspredicate::parser::expression_def];

  bool r = x3::phrase_parse(it, end, parser, x3::ascii::space, _expr);

  if (r && it == end)
  {
    std::ostringstream out;
    lspredicate::ast::print(out, _expr);
    spdlog::debug("Predicate: {0}", out.str());
  }
  else
  {
    //spdlog::critical("Failed to parse predicate ad: \"{0}\"", std::string(it, end));
    throw std::runtime_error(
	fmt::format("Failed to parse predicate ad: \"{0}\"", std::string(it, end))
	);
  }
}

bool lsp::predicate::CmdlExpression::operator()(const lsp_event_t& event) const
{
  return lsp::predicate::evaluate(event, _expr);
}

bool lsp::predicate::CmdlExpression::operator()(const fan_event_t& event) const
{
  return fan::predicate::evaluate(event, _expr);
}

// ----------------------------------------------------------------------------
