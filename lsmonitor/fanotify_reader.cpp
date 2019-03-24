#include "fanotify_reader.h"
#include <poll.h>

std::atomic_bool fan::Reader::stopping{};

fan::Reader::~Reader()
{
  if (_fad > 0)
    close(_fad);
}

void fan::Reader::operator()()
{
  _fad = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK, O_RDONLY | O_LARGEFILE);
  if (_fad == -1)
  {
    int err = errno;
    throw std::runtime_error(
	fmt::format("Unable init fanotify subscription: {0} - {1}", err, ::strerror(err))
	);

  }
  spdlog::debug("{0}: marking '{1}'", __PRETTY_FUNCTION__, _path.c_str());
  if (fanotify_mark(_fad, FAN_MARK_ADD | FAN_MARK_MOUNT, FAN_OPEN | FAN_CLOSE_WRITE, AT_FDCWD, _path.c_str()) == -1)
  {
    int err = errno;
    throw std::runtime_error(
	fmt::format("Unable mark fanotify subscription: {0} - {1}", err, ::strerror(err))
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
    int fdRead = poll(fds, 1, 100); // every 100 milliseconds
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

      int err = errno;
      throw std::runtime_error(
	  fmt::format("Unable poll fanotify events: {0} - {1}", err, ::strerror(err))
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
	      "Mismatch of fanotify metadata version: {0} vs {1} expected."
	      , metadata->vers
	      , FANOTIFY_METADATA_VERSION
	      )
	    );
      }
      if (metadata->fd >= 0)
      {
	if (!stopping.load())
	{
	  spdlog::debug("{0}: sending [{1:d}] [{2:d}:{3}] {4}"
	      , __PRETTY_FUNCTION__
	      , static_cast<int>(metadata->mask)
	      , metadata->pid
	      , linux::getPidComm(metadata->pid).c_str()
	      , linux::getFdPath(metadata->fd).c_str()
	      );

	  _send(std::make_unique<FileEvent>(metadata));
	}
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
    int err = errno;
    spdlog::error("{0}: unable to read events: {1} - {2}", __PRETTY_FUNCTION__, err, ::strerror(err));
  }
}
