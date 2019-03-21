#pragma once

#include <string>

namespace lsp
{
  struct Event
  {
    virtual std::string toString() const = 0;
    virtual ~Event(){}
  };
}
