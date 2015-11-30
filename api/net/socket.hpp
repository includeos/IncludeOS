#pragma once

#include "packet.hpp"
#include <memory>
#include <string>

namespace net
{
  class Socket
  {
    // we could probably have bind() here too,
    // but it needs to return the correct Socket *class* type
    
    int read(std::string& data);
    int write(const std::string& data);
    void close();
    
  private:
    int transmit(std::shared_ptr<Packet>& pckt);
    int bottom(std::shared_ptr<Packet>& pckt);
    
  };
}
