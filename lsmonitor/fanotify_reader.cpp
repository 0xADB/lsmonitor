#include "fanotify_reader.h"
#include <poll.h>

#include <system_error>

std::atomic_bool fan::Reader::stopping{};

fan::Reader::~Reader()
{
  if (_fad > 0)
    close(_fad);
}

void fan::Reader::operator()(stlab::sender<event_t>&& send, std::string&& path)
{
  _send = std::move(send);
  std::error_code err{};
  _fad = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK, O_RDONLY | O_LARGEFILE);
  if (_fad == -1)
  {
    err.assign(errno, std::system_category());
    throw std::runtime_error(
	fmt::format("Unable to init the fanotify facility: {0} - {1}", err.value(), err.message())
	);

  }
  spdlog::debug("{0}: marking '{1}'", __PRETTY_FUNCTION__, path);
  if (fanotify_mark(_fad, FAN_MARK_ADD | FAN_MARK_MOUNT, FAN_OPEN | FAN_CLOSE_WRITE, AT_FDCWD, path.c_str()) == -1)
  {
    err.assign(errno, std::system_category());
    throw std::runtime_error(
	fmt::format("Unable to mark the fanotify subscription to '{0}': {1} - {2}", path, err.value(), err.message())
	);
  }

  pollEvents(_fad);
  close(_fad);
  _fad = 0;
}

void fan::Reader::pollEvents(int fad)
{
  struct pollfd fds[1] = {{fad, POLLIN, 0}};
  while (!stopping.load())
  {
    int fdRead = poll(fds, 1, -1);
    if (fdRead > 0 && (fds[0].revents & POLLIN))
    {
      handleEvents(fad);
    }
    else if (fdRead == -1)
    {
      if (stopping.load())
	break;
      if (errno == EINTR)
	continue;

      std::error_code err(errno, std::system_category());
      throw std::runtime_error(
	  fmt::format("Unable to poll fanotify events: {0} - {1}", err.value(), err.message())
	  );
    }
  }
}

void fan::Reader::handleEvents(int fad)
{
  using metadata_t = struct fanotify_event_metadata;
  std::vector<metadata_t> metadataBuffer(128);

  auto bytesRead = read(fad, reinterpret_cast<char *>(metadataBuffer.data()), sizeof(metadata_t) * metadataBuffer.size());
  while (!stopping.load() && bytesRead > 0)
  {
    auto metadata = &metadataBuffer[0];
    while (FAN_EVENT_OK(metadata, bytesRead))
    {
      if (metadata->vers != FANOTIFY_METADATA_VERSION)
      {
	throw std::runtime_error(
	    fmt::format(
	      "fanotify metadata version mismatch: {0} vs {1} expected."
	      , metadata->vers
	      , FANOTIFY_METADATA_VERSION
	      )
	    );
      }
      if (metadata->fd >= 0)
      {
	if (!stopping.load())
	  _send(std::make_unique<FileEvent>(metadata));
	close(metadata->fd);
      }
      else if (metadata->fd == FAN_NOFD)
      {
	spdlog::warn("{0}: event buffer overflow", __PRETTY_FUNCTION__);
      }
      metadata = FAN_EVENT_NEXT(metadata, bytesRead);
    }

    if (!stopping.load())
      bytesRead = read(fad, reinterpret_cast<char *>(metadataBuffer.data()), sizeof(metadata_t) * metadataBuffer.size());
  }
  if (bytesRead == -1 && errno != EAGAIN)
  {
    std::error_code err(errno, std::system_category());
    spdlog::error("{0}: unable to read events: {1} - {2}", __PRETTY_FUNCTION__, err.value(), err.message());
  }
}

// ---------------------------------------------------------------------------

// std::atomic_bool fan::v2::Reader::stopping{};
// 
// fan::v2::Reader::~Reader()
// {
//   if (_fd > 0)
//     close(_fd);
// }
// 
// void fan::v2::Reader::operator()(const std::string& path)
// {
//   std::error_code err{};
//   int fad = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK, O_RDONLY | O_LARGEFILE);
//   if (fad == -1)
//   {
//     err.assign(errno, std::system_category());
//     throw std::runtime_error(
// 	fmt::format("Unable to init the fanotify facility: {0} - {1}", err.value(), err.message())
// 	);
//   }
// 
//   spdlog::debug("{0}: marking '{1}'", __PRETTY_FUNCTION__, path.c_str());
//   if (fanotify_mark(fad, FAN_MARK_ADD | FAN_MARK_MOUNT, FAN_OPEN | FAN_CLOSE_WRITE, AT_FDCWD, path.c_str()) == -1)
//   {
//     err.assign(errno, std::system_category());
//     throw std::runtime_error(
// 	fmt::format("Unable to mark the fanotify subscription to '{0}': {1} - {2}", path, err.value(), err.message())
// 	);
//   }
// 
//   std::pair<stlab::sender<int>, stlab::receiver<int>> channel = stlab::channel<int>(stlab::default_executor);
// 
//   auto result = channel.second
//     | [](int fd)
//       {
// 	std::vector<struct fanotify_event_metadata> metadataBuffer;
// 	std::vector<std::unique_ptr<FileEvent>> events;
// 	if (!stopping.load())
// 	{
// 	  metadataBuffer.reserve(128);
// 	  ssize_t bytesRead =
// 	    read(fad, reinterpret_cast<char *>(metadataBuffer.data()), sizeof(metadata_t) * metadataBuffer.size());
// 
// 	  if (!stopping.load() && bytesRead > 0)
// 	  {
// 	    auto metadata = &metadataBuffer[0];
// 	    while (FAN_EVENT_OK(metadata, bytesRead))
// 	    {
// 	      if (metadata->vers != FANOTIFY_METADATA_VERSION)
// 	      {
// 		throw std::runtime_error(
// 		    fmt::format(
// 		      "fanotify metadata version mismatch: {0} vs {1} expected."
// 		      , metadata->vers
// 		      , FANOTIFY_METADATA_VERSION
// 		      )
// 		    );
// 	      }
// 	      if (metadata->fd >= 0)
// 	      {
// 		if (!stopping.load())
// 		{
// 		  spdlog::debug("{0}: sending [{1:d}] [{2:d}:{3}] {4}"
// 		      , __PRETTY_FUNCTION__
// 		      , static_cast<int>(metadata->mask)
// 		      , metadata->pid
// 		      , linux::getPidComm(metadata->pid).c_str()
// 		      , linux::getFdPath(metadata->fd).c_str()
// 		      );
// 
// 		  events.emplace_back(std::make_unique<FileEvent>(metadata));
// 		}
// 		close(metadata->fd);
// 	      }
// 	      else if (metadata->fd == FAN_NOFD)
// 	      {
// 		spdlog::warn("{0}: event buffer overflow", __PRETTY_FUNCTION__);
// 	      }
// 	      metadata = FAN_EVENT_NEXT(metadata, bytesRead);
// 	    }
// 	  }
// 	  if (bytesRead == -1 && errno != EAGAIN)
// 	  {
// 	    std::error_code err(errno, std::system_category());
// 	    throw std::runtime_error(
// 		fmt::format("{0}: unable to read events: {1} - {2}", __PRETTY_FUNCTION__, err.value(), err.message())
// 		);
// 	  }
// 	}
// 	return events;
//       };
// 
//   receive.set_ready();
// 
//   struct pollfd fds[1] = {{fad, POLLIN, 0}};
//   while (!stopping.load())
//   {
//     int fdRead = poll(fds, 1, -1);
//     if (fdRead > 0 && (fds[0].revents & POLLIN))
//     {
//       channel.first(fad);
//     }
//     else if (fdRead == -1)
//     {
//       if (stopping.load())
// 	break;
//       if (errno == EINTR)
// 	continue;
// 
//       std::error_code err(errno, std::system_category());
//       throw std::runtime_error(
// 	  fmt::format("Unable to poll fanotify events: {0} - {1}", err.value(), err.message())
// 	  );
//     }
//   }
// }
