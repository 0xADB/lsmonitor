#include "filter.h"
#include "stringify.h"
#include "variant.h"
#include "file_event_predicate.h"
#include "container.h"
#include "broadcast.h"

#include "stlab/concurrency/channel.hpp"
#include "stlab/concurrency/default_executor.hpp"
#include "stlab/concurrency/immediate_executor.hpp"

#include <thread>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>
#include <iterator>

namespace lsp
{
  struct FilenameEqual
  {
    template<typename L, typename R>
      std::enable_if_t<
        std::is_same_v<
	  typename L::value_type
	  , typename std::iterator_traits<typename L::iterator>::value_type
	  >
        && std::is_same_v<
	  typename R::value_type
	  , typename std::iterator_traits<typename R::iterator>::value_type
	  >
        , bool
        >
        operator()(const L& l, const R& r) const
      {
        return std::equal(
            std::begin(l), std::end(l)
            , std::begin(r), std::end(r)
            , [](const auto& l, const auto& r) {return (l->filename == r->filename);}
            );
      }

    template<typename L, typename R>
      bool operator()(const L& l, const R& r) const
      {
	spdlog::debug("{0} vs {1}", l->filename, r->filename);
        return (l->filename == r->filename);
      }

  };

  struct FilenameLess
  {
    template<typename L, typename R>
      bool operator()(const L& l, const R& r) const
      {
        return (l->filename < r->filename);
      }
  };
} //lsp

void printStats(const std::map<std::string, size_t>& stats, int width = 70)
{
  if (stats.empty()) return;
  std::cout << "Stats:\n";
  for (const auto& s : stats)
    std::cout << "  " << std::setw(width) << std::left << std::setfill('.') << s.first.c_str() << ' ' << s.second << '\n';
  std::cout << std::endl;
}

template<typename Predicate>
void SourceManager::only(lsp::Reader&& reader, Predicate&& predicate)
{
  using event_t = std::unique_ptr<lsp::FileEvent>;
  stlab::sender<event_t> sender;
  stlab::receiver<event_t> receiver;
  std::tie(sender, receiver) = stlab::channel<event_t>(stlab::default_executor);

  auto r = receiver
    | [](event_t event)
      {
	spdlog::debug("{0} | lsp: {1} : pid[{2}] : cred [{3}({4}):{5}({6})] : op[{7}] : {8}"
	    , "only"
	    , event->process
	    , event->pcred.tgid
	    , event->user
	    , event->pcred.uid
	    , event->group
	    , event->pcred.gid
	    , static_cast<int>(event->code)
	    , event->filename
	    );
	return event;
      }
    | lsp::filter<event_t, Predicate>{predicate}
    | lsp::stringify{}
    | [](std::string&& str) {spdlog::info("{0} | {1}", "only", str);}; // last one shouldn't return

  receiver.set_ready();

  reader.operator()(std::move(sender)); // listen and send
}

template<typename Predicate>
void SourceManager::only(fan::Reader&& reader, Predicate&& predicate)
{
  using event_t = std::unique_ptr<fan::FileEvent>;
  stlab::sender<event_t> sender;
  stlab::receiver<event_t> receiver;
  std::tie(sender, receiver) = stlab::channel<event_t>(stlab::default_executor);

  auto r = receiver
    | [](event_t event)
      {
	spdlog::debug("{0} | fan: {1} : pid[{2}] : cred [{3}:{4}] : op[{5}] : {6}"
	    , "only"
	    , event->process
	    , event->pid
	    , event->uid
	    , event->gid
	    , static_cast<int>(event->code)
	    , event->filename
	    );
	return event;
      }
    | lsp::filter<event_t, Predicate>{predicate}
    | lsp::stringify{}
    | [](std::string&& str) {spdlog::info("{0} | {1}", "only", str);}; // last one shouldn't return

  receiver.set_ready();

  reader.operator()(std::move(sender), std::string("/home/")); // listen and send
}

template<typename Predicate>
void SourceManager::any(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, Predicate&& predicate)
{
  using lsp_event_t = std::unique_ptr<lsp::FileEvent>;
  using fan_event_t = std::unique_ptr<fan::FileEvent>;

  auto lsp_channel = stlab::channel<lsp_event_t>(stlab::default_executor);
  auto fan_channel = stlab::channel<fan_event_t>(stlab::default_executor);

  auto lsp_r =
    lsp_channel.second
    | [](lsp_event_t event)
      {
	spdlog::debug("{0} | lsp: started: {1} : pid[{2}] : {3} : {4}"
	    , "any"
	    , event->process
	    , event->pcred.tgid
	    , static_cast<int>(event->code)
	    , event->filename
	    );
	return event;
      }
    | lsp::filter<lsp_event_t, Predicate>{predicate}
    | lsp::stringify{}
    | [](std::string&& str) {spdlog::info("{0} | lsp: {1}", "any", str);};

  auto fan_r =
    fan_channel.second
    | [](fan_event_t event)
      {
	spdlog::debug("{0} | fan: started: {1} : pid[{2}] : {3} : {4}"
	    , "any"
	    , event->process
	    , event->pid
	    , static_cast<int>(event->code)
	    , event->filename
	    );
	return event;
      }
    | lsp::filter<fan_event_t, Predicate>{predicate}
    | lsp::stringify{}
    | [](std::string&& str) {spdlog::info("{0} | fan: {1}", "any", str);};

  lsp_channel.second.set_ready();
  fan_channel.second.set_ready();

  std::thread lsp_thread(std::move(lsp_reader), std::move(lsp_channel.first));
  std::thread fan_thread(std::move(fan_reader), std::move(fan_channel.first), std::string("/home/"));

  fan_thread.join();
  lsp_thread.join();
}

template<typename Predicate>
void SourceManager::count_strings(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, Predicate&& predicate)
{
  using lsp_event_t = std::unique_ptr<lsp::FileEvent>;
  stlab::sender<lsp_event_t> lsp_send;
  stlab::receiver<lsp_event_t> lsp_receive;

  using fan_event_t = std::unique_ptr<fan::FileEvent>;
  stlab::sender<fan_event_t> fan_send;
  stlab::receiver<fan_event_t> fan_receive;

  std::tie(lsp_send, lsp_receive) = stlab::channel<lsp_event_t>(stlab::default_executor);
  std::tie(fan_send, fan_receive) = stlab::channel<fan_event_t>(stlab::default_executor);

  std::map<std::string, size_t> stats;

  auto lsp_r =
    lsp_receive
    | [](lsp_event_t event)
      {
        spdlog::debug("{0} | lsp: started: {1} : pid[{2}] : {3} : {4}"
            , "count_strings"
            , event->process
            , event->pcred.tgid
            , static_cast<int>(event->code)
            , event->filename
            );
        return event;
      }
    | lsp::filter<lsp_event_t, Predicate>{predicate}
    | lsp::stringify{};

  auto fan_r =
    fan_receive
    | [](fan_event_t event)
      {
        spdlog::debug("{0} | fan: started: {1} : pid[{2}] : {3} : {4}"
            , "count_strings"
            , event->process
            , event->pid
            , static_cast<int>(event->code)
            , event->filename
            );
        return event;
      }
    | lsp::filter<fan_event_t, Predicate>{predicate}
    | lsp::stringify{};

  ctl::broadcast broadcast;
  broadcast.setup();

  auto merged = stlab::merge_channel<stlab::unordered_t>(stlab::default_executor
      , [](std::string&& s) {return s;}
      , std::move(lsp_r)
      , std::move(fan_r)
      )
    | [&stats, &broadcast](std::string&& str)
      {
	spdlog::info("{0} | {1}", "count_strings", str);
	stats[str]++;
	broadcast.await(std::move(str));
      };


  lsp_receive.set_ready();
  fan_receive.set_ready();

  std::thread br_thread(&ctl::broadcast::listen, &broadcast);
  std::thread lsp_thread(std::move(lsp_reader), std::move(lsp_send));
  std::thread fan_thread(std::move(fan_reader), std::move(fan_send), std::string("/home/"));

  fan_thread.join();
  lsp_thread.join();
  br_thread.join();

  printStats(stats, 125);

}

template<typename Predicate>
void SourceManager::intersection(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, Predicate&& predicate)
{
  using lsp_event_t = std::unique_ptr<lsp::FileEvent>;
  using fan_event_t = std::unique_ptr<fan::FileEvent>;

  auto lsp_channel = stlab::channel<lsp_event_t>(stlab::default_executor);
  auto fan_channel = stlab::channel<fan_event_t>(stlab::default_executor);

  auto lsp_r =
    lsp_channel.second
    | [](auto event)
      {
        spdlog::debug("{0} | lsp: started: {1} : pid[{2}] : {3} : {4}"
            , "int"
            , event->process
            , event->pcred.tgid
            , static_cast<int>(event->code)
            , event->filename
            );
        return event;
      }
    | lsp::filter<lsp_event_t, Predicate>{predicate};

  auto fan_r =
    fan_channel.second
    | [](auto event)
      {
        spdlog::debug("{0} | fan: started: {1} : pid[{2}] : {3} : {4}"
            , "int"
            , event->process
            , event->pid
            , static_cast<int>(event->code)
            , event->filename
            );
        return event;
      }
    | lsp::filter<fan_event_t, Predicate>{predicate};

  auto combined_channel =
    stlab::zip_with(stlab::default_executor
      , lsp::adjacent_if<lsp::FilenameEqual, lsp_event_t, fan_event_t>{lsp::FilenameEqual{}}
      , std::move(lsp_r)
      , std::move(fan_r)
      )
    | [](auto&& event_variant)
      {
        spdlog::debug("{0} | finish: {1} : variant: {2}", "int", __PRETTY_FUNCTION__, event_variant.index());
        if (event_variant.index() == 0) // lsp_event_t
        {
          auto event = std::move(std::get<0>(event_variant));
          spdlog::info("{0} | lsp: {1}[{2}]: {3}", "int", event->process, event->pcred.tgid, event->filename);
        }
        else if (event_variant.index() == 1) // fan_event_t
        {
          auto event = std::move(std::get<1>(event_variant));
          spdlog::info("{0} | fan: {1}[{2}]: {3}", "int", event->process, event->pid, event->filename);
        }
      };

  lsp_channel.second.set_ready();
  fan_channel.second.set_ready();

  std::thread lsp_thread(std::move(lsp_reader), std::move(lsp_channel.first));
  std::thread fan_thread(std::move(fan_reader), std::move(fan_channel.first), std::string("/home/"));

  fan_thread.join();
  lsp_thread.join();
}

template<typename Predicate>
void SourceManager::difference(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, Predicate&& predicate)
{
  using lsp_event_t = std::unique_ptr<lsp::FileEvent>;
  using fan_event_t = std::unique_ptr<fan::FileEvent>;

  auto lsp_channel = stlab::channel<lsp_event_t>(stlab::default_executor);
  auto fan_channel = stlab::channel<fan_event_t>(stlab::default_executor);

  std::map<std::string, size_t> stats;

  auto lsp_r =
    lsp_channel.second
    | [](auto event)
      {
	spdlog::debug("{0} | lsp: started: {1} : pid[{2}] : {3} : {4}"
	    , "diff"
	    , event->process
	    , event->pcred.tgid
	    , static_cast<int>(event->code)
	    , event->filename
	    );
	return event;
      }
    | lsp::filter<lsp_event_t, Predicate>{predicate};

  auto fan_r =
    fan_channel.second
    | [](auto event)
      {
	spdlog::debug("{0} | fan: started: {1} : pid[{2}] : {3} : {4}"
	    , "diff"
	    , event->process
	    , event->pid
	    , static_cast<int>(event->code)
	    , event->filename
	    );
	return event;
      }
    | lsp::filter<fan_event_t, Predicate>{predicate};

  auto combined_channel = stlab::zip_with(stlab::default_executor
      , lsp::adjacent_if<
	  decltype(std::not_fn(lsp::FilenameEqual{}))
	  , lsp_event_t
	  , fan_event_t
	  >{std::not_fn(lsp::FilenameEqual{})}
      , std::move(lsp_r)
      , std::move(fan_r)
      )
    | [&stats](auto&& event_variant)
      {
        spdlog::debug("{0} | finish: {1} : variant: {2}", "int", __PRETTY_FUNCTION__, event_variant.index());
	if (event_variant.index() == 0) // lsp_event_t
	{
	  auto event = std::move(std::get<0>(event_variant));
	  spdlog::info("{0} | lsp: {1}[{2}]: {3}", "diff", event->process, event->pcred.tgid, event->filename);
	  stats[event->filename]++;
	}
	else if (event_variant.index() == 1) // fan_event_t
	{
	  auto event = std::move(std::get<1>(event_variant));
	  spdlog::info("{0} | fan: {1}[{2}]: {3}", "diff", event->process, event->pid, event->filename);
	  stats[event->filename]++;
	}
      };

  lsp_channel.second.set_ready();
  fan_channel.second.set_ready();

  std::thread lsp_thread(std::move(lsp_reader), std::move(lsp_channel.first));
  std::thread fan_thread(std::move(fan_reader), std::move(fan_channel.first), std::string("/home/"));

  fan_thread.join();
  lsp_thread.join();

  printStats(stats);

}

template<typename Predicate>
void SourceManager::buffered_difference(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, Predicate&& predicate, size_t buffer_size)
{
  using lsp_event_t = std::unique_ptr<lsp::FileEvent>;
  using fan_event_t = std::unique_ptr<fan::FileEvent>;

  auto lsp_channel = stlab::channel<lsp_event_t>(stlab::default_executor);
  auto fan_channel = stlab::channel<fan_event_t>(stlab::default_executor);

  using lsp_buffer_t = std::multiset<lsp_event_t>;
  using fan_buffer_t = std::multiset<fan_event_t>;

  std::map<std::string, size_t> stats;

  auto lsp_r =
    lsp_channel.second
    | [](auto event)
      {
	spdlog::debug("{0} | lsp: started: {1} : pid[{2}] : {3} : {4}"
	    , "diff"
	    , event->process
	    , event->pcred.tgid
	    , static_cast<int>(event->code)
	    , event->filename
	    );
	return event;
      }
    | lsp::filter<lsp_event_t, Predicate>{predicate}
    | lsp::queue<lsp_buffer_t>(buffer_size);

  auto fan_r =
    fan_channel.second
    | [](auto event)
      {
	spdlog::debug("{0} | fan: started: {1} : pid[{2}] : {3} : {4}"
	    , "diff"
	    , event->process
	    , event->pid
	    , static_cast<int>(event->code)
	    , event->filename
	    );
	return event;
      }
    | lsp::filter<fan_event_t, Predicate>{predicate}
    | lsp::queue<fan_buffer_t>(buffer_size);

  auto combined_channel = stlab::zip_with(stlab::default_executor
      , lsp::adjacent_if<
	  decltype(std::not_fn(lsp::FilenameEqual{}))
	  , lsp_event_t
	  , fan_event_t
	  >{std::not_fn(lsp::FilenameEqual{})}
      , std::move(lsp_r)
      , std::move(fan_r)
      )
    | [&stats](auto&& event_variant)
      {
        spdlog::debug("{0} | finish: {1} : variant: {2}", "int", __PRETTY_FUNCTION__, event_variant.index());
	if (event_variant.index() == 0) // lsp_event_t
	{
	  auto event = std::move(std::get<0>(event_variant));
	  spdlog::info("{0} | lsp: {1}[{2}]: {3}", "diff", event->process, event->pcred.tgid, event->filename);
	  stats[event->filename]++;
	}
	else if (event_variant.index() == 1) // fan_event_t
	{
	  auto event = std::move(std::get<1>(event_variant));
	  spdlog::info("{0} | fan: {1}[{2}]: {3}", "diff", event->process, event->pid, event->filename);
	  stats[event->filename]++;
	}
      };
  lsp_channel.second.set_ready();
  fan_channel.second.set_ready();

  std::thread lsp_thread(std::move(lsp_reader), std::move(lsp_channel.first));
  std::thread fan_thread(std::move(fan_reader), std::move(fan_channel.first), std::string("/home/"));

  fan_thread.join();
  lsp_thread.join();

  printStats(stats);

}
