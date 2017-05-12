
#include "transport.hpp"

#include <common>

namespace uplink {

Header Header::parse(const char* data)
{
  Expects(data != nullptr);
  return Header{
    static_cast<Transport_code>(data[0]), 
    *(reinterpret_cast<const uint32_t*>(&data[1]))
  };
}

Transport_parser::Transport_parser(Transport_complete cb)
  : on_complete{std::move(cb)}, on_header{nullptr}, transport_{nullptr}
{
  Expects(on_complete != nullptr);
}

void Transport_parser::parse(const char* data, size_t len)
{
  printf("Parsing data %lu\n", len);
  if(transport_ != nullptr)
  {
    transport_->load_cargo(data, len);
  }
  else
  {
    transport_ = std::make_unique<Transport>(Header::parse(data));
    
    if(on_header)
      on_header(transport_->header());

    len -= sizeof(Header);

    transport_->load_cargo(data + sizeof(Header), len);
  }

  if(transport_->is_complete())
    on_complete(std::move(transport_));
}

}

