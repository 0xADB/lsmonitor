#pragma once

#include "spdlog/spdlog.h"

#include <variant>
#include <optional>

#include <stlab/concurrency/channel.hpp>

namespace lsp
{
  template<typename... Ts>
    struct variant
    {
      using value_type = std::variant<Ts...>;

      std::variant<Ts...> _result{};
      stlab::process_state_scheduled _state = stlab::await_forever;

      template<typename T>
	void await(T&& value)
	{
	  _result = std::forward(value);
	  _state = stlab::yield_immediate;
	}

      auto yield()
      {
	auto result = std::move(_result);
	_state = stlab::await_forever;
	return result;
      }

      void set_error(std::exception_ptr error)
      {
	try
	{
	  if (error)
	    std::rethrow_exception(error);
	}
	catch (const std::exception& e)
	{
	  spdlog::critical("{0} : {1}", __PRETTY_FUNCTION__, e.what());
	  throw;
	}      }

      auto state() const {return _state;}
    };

  template<typename BinaryPredicate, typename... Ts>
    struct adjacent_if
    {
      using value_type = std::variant<Ts...>;

      BinaryPredicate _predicate{};
      std::vector<value_type> _results{};
      stlab::process_state_scheduled _state = stlab::await_forever;

      void push_adjacent_if(){}

      template<typename... Args>
	void push_adjacent_if(Args&&...);

      template<typename L, typename R>
	void push_adjacent_if(L&& l, R&& r)
	{
	  if (_predicate(l, r))
	  {
	    _results.emplace_back(value_type(std::forward<L>(l)));
	    _results.emplace_back(value_type(std::forward<R>(r)));
	  }
	}

      template<typename L, typename R, typename... Args>
	void push_adjacent_if(L&& l, R&& r, Args&&... args)
	{
	  if (_predicate(l, r))
	    _results.emplace_back(value_type(std::forward<L>(l)));
	  push_adjacent_if(std::forward<R>(r), std::forward<Args>(args)...);
	}

      void await(Ts&&... values)
      {
        _results.reserve(sizeof...(Ts));
        push_adjacent_if(std::forward<Ts>(values)...);
        if (!_results.empty())
	{
	  spdlog::debug("{0}: {1}", __PRETTY_FUNCTION__, _results.size());
          _state = stlab::yield_immediate;
	}
	else
	  _state = stlab::await_forever;
      }

      auto yield()
      {
	value_type result;
	if (!_results.empty())
	{
	  result = std::move(_results.back());
	  _results.pop_back();
	  _state = (_results.empty() ? stlab::await_forever : stlab::yield_immediate);
	  spdlog::debug("{0}", __PRETTY_FUNCTION__);
	}
	return result;
      }

      void set_error(std::exception_ptr error)
      {
	try
	{
	  if (error)
	    std::rethrow_exception(error);
	}
	catch (const std::exception& e)
	{
	  spdlog::critical("{0} : {1}", __PRETTY_FUNCTION__, e.what());
	  throw;
	}
      }

      auto state() const {return _state;}
    };

  template<typename L, typename R, typename Less>
    struct set_difference
    {
      using value_type = std::variant<typename L::value_type, typename R::value_type>;

      Less _less{};
      std::vector<value_type> _results{};
      stlab::process_state_scheduled _state = stlab::await_forever;

      void await(L&& l, R&& r)
      {
        _results.reserve(l.size() + r.size());
	auto lit = std::begin(l);
	auto lEndIt = std::end(l);

	auto rit = std::begin(r);
	auto rEndIt = std::end(r);

	while (lit != lEndIt && rit != rEndIt)
	{
	  if (_less(*lit, *rit))
	  {
	    _results.emplace_back(value_type(std::move(l.extract(lit).value())));
	    lit = std::begin(l);
	  }
	  else if (_less(*rit, *lit))
	  {
	    _results.emplace_back(value_type(std::move(r.extract(rit).value())));
	    rit = std::begin(r);
	  }
	  else
	  {
	    l.extract(lit);
	    lit = std::begin(l);

	    r.extract(rit);
	    rit = std::begin(r);
	  }
	}

        if (!_results.empty())
          _state = stlab::yield_immediate;
	else
	  _state = stlab::await_forever;
      }

      value_type yield()
      {
	value_type result;
	if (!_results.empty())
	{
	  result = std::move(_results.back());
	  _results.pop_back();
	}
	_state = (_results.empty() ? stlab::await_forever : stlab::yield_immediate);
	return result;
      }

      void set_error(std::exception_ptr error)
      {
	try
	{
	  if (error)
	    std::rethrow_exception(error);
	}
	catch (const std::exception& e)
	{
	  spdlog::critical("{0} : {1}", __PRETTY_FUNCTION__, e.what());
	  throw;
	}
      }

      auto state() const {return _state;}
    };
} // lsp
