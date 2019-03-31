#include "source_manager.h"

#include "filter.h"
#include "stringify.h"
#include "variant.h"
#include "file_event_predicate.h"

#include "stlab/concurrency/channel.hpp"
#include "stlab/concurrency/default_executor.hpp"
#include "stlab/concurrency/immediate_executor.hpp"

#include <thread>
#include <type_traits>
#include <functional>
#include <iomanip>
#include <iostream>

namespace lsp
{
  struct SameFilename
  {
    template<typename L, typename R>
      bool operator()(const L& l, const R& r) const
      {
	spdlog::debug("comparing filenames: {0} == {1}", l->filename, r->filename);
	return (l->filename == r->filename);
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

void SourceManager::only(lsp::Reader&& reader, lsp::predicate::CmdlSimpleConjunctive&& predicate)
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
    | lsp::filter<event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate} // TODO: multievent predicate
    | lsp::stringify{}
    | [](std::string&& str) {spdlog::info("{0} | {1}", "only", str);}; // last one shouldn't return

  receiver.set_ready();

  reader.operator()(std::move(sender)); // listen and send
}

void SourceManager::only(fan::Reader&& reader, lsp::predicate::CmdlSimpleConjunctive&& predicate)
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
    | lsp::filter<event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate}
    | lsp::stringify{}
    | [](std::string&& str) {spdlog::info("{0} | {1}", "only", str);}; // last one shouldn't return

  receiver.set_ready();

  reader.operator()(std::move(sender), std::string("/home/")); // listen and send
}

void SourceManager::any(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, lsp::predicate::CmdlSimpleConjunctive&& predicate)
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
    | lsp::filter<lsp_event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate}
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
    | lsp::filter<fan_event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate}
    | lsp::stringify{}
    | [](std::string&& str) {spdlog::info("{0} | fan: {1}", "any", str);};

  lsp_channel.second.set_ready();
  fan_channel.second.set_ready();

  std::thread lsp_thread(std::move(lsp_reader), std::move(lsp_channel.first));
  std::thread fan_thread(std::move(fan_reader), std::move(fan_channel.first), std::string("/home/"));

  fan_thread.join();
  lsp_thread.join();
}

void SourceManager::count_strings(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, lsp::predicate::CmdlSimpleConjunctive&& predicate)
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
            , "all_str"
            , event->process
            , event->pcred.tgid
            , static_cast<int>(event->code)
            , event->filename
            );
        return event;
      }
    | lsp::filter<lsp_event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate}
    | lsp::stringify{};

  auto fan_r =
    fan_receive
    | [](fan_event_t event)
      {
        spdlog::debug("{0} | fan: started: {1} : pid[{2}] : {3} : {4}"
            , "all_str"
            , event->process
            , event->pid
            , static_cast<int>(event->code)
            , event->filename
            );
        return event;
      }
    | lsp::filter<fan_event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate}
    | lsp::stringify{};

  auto merged = stlab::merge_channel<stlab::unordered_t>(stlab::default_executor
      , [](std::string&& s) {return s;}
      , std::move(lsp_r)
      , std::move(fan_r)
      )
    | [&stats](std::string str)
      {
	spdlog::info("{0} | {1}", "all_str", str);
	stats[str]++;
      };

  lsp_receive.set_ready();
  fan_receive.set_ready();

  std::thread lsp_thread(std::move(lsp_reader), std::move(lsp_send));
  std::thread fan_thread(std::move(fan_reader), std::move(fan_send), std::string("/home/"));

  fan_thread.join();
  lsp_thread.join();

  printStats(stats, 125);

}

void SourceManager::intersection(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, lsp::predicate::CmdlSimpleConjunctive&& predicate)
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
    | lsp::filter<lsp_event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate};

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
    | lsp::filter<fan_event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate};

  auto combined_channel =
    stlab::zip_with(stlab::default_executor
      , lsp::adjacent_if<lsp::SameFilename, lsp_event_t, fan_event_t>{lsp::SameFilename{}}
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

void SourceManager::difference(lsp::Reader&& lsp_reader, fan::Reader&& fan_reader, lsp::predicate::CmdlSimpleConjunctive&& predicate)
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
    | lsp::filter<lsp_event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate};

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
    | lsp::filter<fan_event_t, lsp::predicate::CmdlSimpleConjunctive>{predicate};

  auto combined_channel = stlab::zip_with(stlab::default_executor
      , lsp::adjacent_if<
	  decltype(std::not_fn(lsp::SameFilename{}))
	  , lsp_event_t
	  , fan_event_t
	  >{std::not_fn(lsp::SameFilename{})}
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
