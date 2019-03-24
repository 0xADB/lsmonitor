#pragma once

#include "function_traits.h"
#include <type_traits>

#include <stlab/concurrency/channel.hpp>
#include <stlab/concurrency/default_executor.hpp>

#include <memory>

namespace lsp
{

  template <typename EventT, typename Predicate>
    struct filter
    {
      Predicate _predicate{};
      EventT _event{};
      stlab::process_state_scheduled _state = stlab::await_forever;

      void await(EventT&& event)
      {
	if (_predicate(event))
	{
	  _event = std::move(event);
	  _state = stlab::yield_immediate;
	}
      }

      EventT yield()
      {
	EventT event = std::move(_event);
	_event = EventT();
	_state = stlab::await_forever;
	return event;
      }

      auto state() const
      {
	return _state;
      }
    };

  template<
    typename Predicate
    , typename EventT = std::remove_cv_t<std::remove_reference_t<
	    typename function_traits<Predicate>::template argument<0>::type
	    >>
    >
    auto make_filter(Predicate&& p)
    {
      return filter<EventT, Predicate>{std::forward<Predicate>(p)};
    }

} // namespace lsp
