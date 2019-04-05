#pragma once

#include <stlab/concurrency/channel.hpp>
#include <stlab/concurrency/default_executor.hpp>

#include <type_traits>

namespace lsp
{

  template <typename Container>
    struct container
    {
      size_t _capacity = 1;
      Container _container{};
      stlab::process_state_scheduled _state = stlab::await_forever;

      container(size_t capacity = 1)
	: _capacity(capacity)
      {}

      // sets
      template<typename T>
	std::enable_if_t<
	  std::is_same_v<T, typename Container::node_type::value_type>
	  , void
	  >
	await(T&& value)
	{
	  _container.reserve(_capacity);
	  _container.insert(std::move(value));
	  if (_container.size() == _capacity)
	    _state = stlab::yield_immediate;
	  else
	    _state = stlab::await_forever;
	}

      // vector
      template<typename T>
	std::enable_if_t<
	  std::is_same_v<T, typename Container::value_type>
	  , void
	  >
	await(T&& value)
	{
	  _container.reserve(_capacity);
	  _container.emplace_back(std::move(value));
	  if (_container.size() == _capacity)
	    _state = stlab::yield_immediate;
	  else
	    _state = stlab::await_forever;
	}

      auto yield()
      {
	auto value = std::move(_container);
	spdlog::debug("{0}: push container of {1} elements", __PRETTY_FUNCTION__, value.size());
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

  // template <typename Container, typename Value = void>
  //   struct queue{};

  // push_back()-based
  // template <typename Container>
  //   struct queue<
  //     Container
  //     , std::enable_if_t<
  //         std::is_same_v<
  //           typename std::iterator_traits<typename Container::iterator>::value_type
  //           , typename Container::value_type
  //           >
  //         , typename Container::value_type
  //         >
  //     >
  //   {
  //     using value_type = typename Container::value_type;

  //     size_t _capacity = 1;
  //     Container _container{};
  //     stlab::process_state_scheduled _state = stlab::await_forever;

  //     queue(size_t capacity = 1)
  //       : _capacity(capacity)
  //     {}

  //     void await(value_type&& value)
  //       {
  //         _container.reserve(_capacity);
  //         _container.emplace_back(std::move(value));
  //         if (_container.size() == _capacity)
  //           _state = stlab::yield_immediate;
  //         else
  //           _state = stlab::await_forever;
  //       }

  //     auto yield()
  //     {
  //       auto it = std::begin(_container);
  //       auto value = std::move(*it);
  //       _container.pop_front();
  //       if (_container.empty())
  //         _state = stlab::await_forever;
  //       return value;
  //     }

  //     auto state() const
  //     {
  //       return _state;
  //     }

  //     void set_error(std::exception_ptr error)
  //     {
  //       try
  //       {
  //         if (error)
  //           std::rethrow_exception(error);
  //       }
  //       catch (const std::exception& e)
  //       {
  //         spdlog::critical("{0} : {1}", __PRETTY_FUNCTION__, e.what());
  //         throw;
  //       }
  //     }
  //   };

  // insert-based
  //template <typename Container>
  //  struct queue<
  //    Container
  //    , std::enable_if_t<
  //        std::is_same_v<typename Container::key_type, typename Container::value_type>
  //        , typename Container::value_type
  //        >
  //    >
  template <typename Container>
    struct queue
    {
      using key_type = typename Container::key_type;
      using value_type = typename Container::value_type;

      size_t _capacity = 1;
      Container _container{};
      stlab::process_state_scheduled _state = stlab::await_forever;

      queue(size_t capacity = 1)
	: _capacity(capacity)
      {}

      template<typename T>
      void await(T&& value)
      {
	spdlog::debug("{0}: {1} -> {2} now", __PRETTY_FUNCTION__, value->filename, _container.size());
	_container.insert(std::move(value));
	if (_container.size() == _capacity)
	  _state = stlab::yield_immediate;
	else
	  _state = stlab::await_forever;
      }

      auto yield()
      {
	auto value = std::move(_container.extract(_container.begin()).value());
	if (_container.empty())
	  _state = stlab::await_forever;
	spdlog::debug("{0}: {1} -> {2} left", __PRETTY_FUNCTION__, value->filename, _container.size());
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
}
