#pragma once

#include "function_traits.h"
#include <type_traits>

#include <stlab/concurrency/channel.hpp>
#include <stlab/concurrency/default_executor.hpp>

#include <memory>

namespace lsp
{
  template <typename Value, typename Predicate>
    struct filter
    {
      Predicate _predicate{};
      Value _value{};
      stlab::process_state_scheduled _state = stlab::await_forever;

      template<typename T>
      void await(T&& value)
      {
	spdlog::debug("{0}: {1}", __PRETTY_FUNCTION__, value->filename);
	if (_predicate(value))
	{
	  _value = std::move(value);
	  _state = stlab::yield_immediate;
	}
	else
	  _state = stlab::await_forever;
      }

      auto yield()
      {
	auto value = std::move(_value);
	spdlog::debug("{0}: {1}", __PRETTY_FUNCTION__, value->filename);
	_state = stlab::await_forever;
	return value;
      }

      auto state() const
      {
	return _state;
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
    };

  template<
    typename Predicate
    , typename Value = std::remove_cv_t<
	    std::remove_reference_t<
	      typename function_traits<Predicate>::template argument<0>::type
	      >
	    >
    >
    auto make_filter(Predicate&& p)
    {
      return filter<Value, Predicate>{std::forward<Predicate>(p)};
    }

} // namespace lsp
