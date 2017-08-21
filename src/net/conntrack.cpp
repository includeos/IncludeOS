// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <net/conntrack.hpp>

namespace net {

Conntrack::Entry* Conntrack::simple_track_out(Quadruple q, const Protocol proto)
{
  // find the entry
  auto* entry = out(q, proto);

  // if none, add new and return
  if(entry == nullptr)
  {
    entry = add_entry(q, proto, Seen::OUT);
    entry->timeout = RTC::now() + timeout_new.count();
    return entry;
  }

  // if this is a reply
  if(entry->direction == Seen::IN)
  {
    entry->direction  = Seen::BOTH;
    entry->state      = State::ESTABLISHED;
  }

  entry->timeout = RTC::now() +
    ((entry->state == State::ESTABLISHED) ? timeout_est.count() : timeout_new.count());

  return entry;
}

Conntrack::Entry* Conntrack::simple_track_in(Quadruple q, const Protocol proto)
{
  // find the entry
  auto* entry = in(q, proto);

  // if none, add new and return
  if(entry == nullptr)
  {
    q.swap(); // swap due to the nature of add_entry
    entry = add_entry(q, proto, Seen::OUT);
    entry->timeout = RTC::now() + timeout_new.count();
    return entry;
  }

  // if this is a reply
  if(entry->direction == Seen::OUT)
  {
    entry->direction  = Seen::BOTH;
    entry->state      = State::ESTABLISHED;
  }

  entry->timeout = RTC::now() +
    ((entry->state == State::ESTABLISHED) ? timeout_est.count() : timeout_new.count());

  return entry;
}

Conntrack::Entry* dumb_out(Conntrack& ct, Quadruple q, const PacketIP4& pkt)
{ return ct.simple_track_out(std::move(q), pkt.ip_protocol()); }

Conntrack::Entry* dumb_in(Conntrack& ct, Quadruple q, const PacketIP4& pkt)
{ return  ct.simple_track_in(std::move(q), pkt.ip_protocol()); }

Conntrack::Conntrack()
 : tcp_out{&dumb_out},
   tcp_in{&dumb_in}
{}

Conntrack::Entry* Conntrack::out(const Quadruple& quad, const Protocol proto) const
{
  auto it = out_lookup.find({quad, proto});

  if(it != out_lookup.end())
    return it->second;

  return nullptr;
}

Conntrack::Entry* Conntrack::in(const Quadruple& quad, const Protocol proto) const
{
  auto it = in_lookup.find({quad, proto});

  if(it != in_lookup.end())
    return it->second;

  return nullptr;
}

Quadruple Conntrack::get_quadruple(const PacketIP4& pkt)
{
  const auto* ports = reinterpret_cast<const uint16_t*>(pkt.ip_data().data());
  uint16_t src_port = ntohs(*ports);
  uint16_t dst_port = ntohs(*(ports + 1));

  return {{pkt.ip_src(), src_port}, {pkt.ip_dst(), dst_port}};
}

Conntrack::Entry* Conntrack::track_out(const PacketIP4& pkt)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      return tcp_out(*this, get_quadruple(pkt), pkt);

    case Protocol::UDP:
      return simple_track_out(get_quadruple(pkt), pkt.ip_protocol());

    case Protocol::ICMPv4:
      return nullptr;

    default:
      return nullptr;
  }
}

Conntrack::Entry* Conntrack::track_in(const PacketIP4& pkt)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      return tcp_in(*this, get_quadruple(pkt), pkt);

    case Protocol::UDP:
      return simple_track_in(get_quadruple(pkt), pkt.ip_protocol());

    case Protocol::ICMPv4:
      return nullptr;

    default:
      return nullptr;
  }
}

Conntrack::Entry* Conntrack::add_entry(
  const Quadruple& quad, const Protocol proto, const Seen direction)
{
  entries.push_back(std::make_unique<Entry>(quad, proto, direction));
  auto* entry = entries.back().get();

  // index OUT
  out_lookup.emplace(std::piecewise_construct,
    std::forward_as_tuple(entry->out, proto),
    std::forward_as_tuple(entry));

  // index IN
  in_lookup.emplace(std::piecewise_construct,
    std::forward_as_tuple(entry->in, proto),
    std::forward_as_tuple(entry));

  return entry;
}

void Conntrack::remove_entry(Entry* entry)
{
  out_lookup.erase({entry->out, entry->proto});
  in_lookup.erase({entry->in, entry->proto});

  on_close(entry);
}

}
