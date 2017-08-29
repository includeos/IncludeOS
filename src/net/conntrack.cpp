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

std::string proto_str(const Protocol proto)
{
  switch(proto) {
    case Protocol::TCP: return "TCP";
    case Protocol::UDP: return "UDP";
    case Protocol::ICMPv4: return "ICMPv4";
    default: return "???";
  }
}

std::string Conntrack::Entry::to_string() const
{
  return "0: " + first.src.to_string() + " " + first.dst.to_string()
    + " 1: " + second.src.to_string() + " " + second.dst.to_string() + " P: " + proto_str(proto);
}

Conntrack::Entry* Conntrack::simple_track_in(Quadruple q, const Protocol proto)
{
  // find the entry
  auto* entry = get(q, proto);

  printf("<Conntrack> Track in SRC: %s - DST: %s\n",
    q.src.to_string().c_str(), q.dst.to_string().c_str());

  // if none, add new and return
  if(entry == nullptr)
  {
    entry = add_entry(q, proto);
    entry->timeout = RTC::now() + timeout_new.count();
    return entry;
  }

  // temp
  printf("<Conntrack> Entry found: %s\n", entry->to_string().c_str());

  if(entry->state == State::NEW and q == entry->second)
  {
    entry->state = State::ESTABLISHED;
    printf("<Conntrack> Assuming ESTABLISHED\n");
  }

  update_timeout(*entry, (entry->state == State::ESTABLISHED) ? timeout_est : timeout_new);

  return entry;
}

Conntrack::Entry* dumb_in(Conntrack& ct, Quadruple q, const PacketIP4& pkt)
{ return  ct.simple_track_in(std::move(q), pkt.ip_protocol()); }

Conntrack::Conntrack()
 : tcp_in{&dumb_in}
{}

Conntrack::Entry* Conntrack::get(const Quadruple& quad, const Protocol proto) const
{
  auto it = entries.find({quad, proto});

  if(it != entries.end())
    return it->second.get();

  return nullptr;
}

Quadruple Conntrack::get_quadruple(const PacketIP4& pkt)
{
  const auto* ports = reinterpret_cast<const uint16_t*>(pkt.ip_data().data());
  uint16_t src_port = ntohs(*ports);
  uint16_t dst_port = ntohs(*(ports + 1));

  return {{pkt.ip_src(), src_port}, {pkt.ip_dst(), dst_port}};
}

Quadruple Conntrack::get_quadruple_icmp(const PacketIP4& pkt)
{
  Expects(pkt.ip_protocol() == Protocol::ICMPv4);

  struct partial_header {
    uint16_t  type_code;
    uint16_t  checksum;
    uint16_t  id;
  };

  // not sure if sufficent
  auto id = reinterpret_cast<const partial_header*>(pkt.ip_data().data())->type_code;

  return {{pkt.ip_src(), id}, {pkt.ip_dst(), id}};
}

Conntrack::Entry* Conntrack::in(const PacketIP4& pkt)
{
  const auto proto = pkt.ip_protocol();
  switch(proto)
  {
    case Protocol::TCP:
      return tcp_in(*this, get_quadruple(pkt), pkt);

    case Protocol::UDP:
      return simple_track_in(get_quadruple(pkt), proto);

    case Protocol::ICMPv4:
      return simple_track_in(get_quadruple_icmp(pkt), proto);

    default:
      return nullptr;
  }
}

Conntrack::Entry* Conntrack::confirm(const PacketIP4& pkt)
{
  const auto proto = pkt.ip_protocol();

  auto quad = [&]()->Quadruple {
    switch(proto)
    {
      case Protocol::TCP:
      case Protocol::UDP:
        return get_quadruple(pkt);

      case Protocol::ICMPv4:
        return get_quadruple_icmp(pkt);

      default:
        return Quadruple();
    }
  }();

  auto quint = Quintuple{quad, proto};
  auto it = unconfirmed.find(quint);

  // if the connection already been confirmed, early return
  if(it == unconfirmed.end())
    return nullptr;

  auto entry = it->second;
  printf("<Conntrack> Confirming %s\n", entry->to_string().c_str());

  // confirm the entry, adding it into the entries map
  entries.emplace(std::piecewise_construct,
  std::forward_as_tuple(entry->first, proto),
  std::forward_as_tuple(entry));

  // also reveresed
  entries.emplace(std::piecewise_construct,
    std::forward_as_tuple(entry->second, proto),
    std::forward_as_tuple(entry));

  // remove it from the list of unconfirmed
  unconfirmed.erase(quint);

  update_timeout(*entry, timeout_new);

  return entry.get();
}

Conntrack::Entry* Conntrack::add_entry(
  const Quadruple& quad, const Protocol proto)
{
  std::shared_ptr<Entry> entry = nullptr;
  // check if there already is a unconfirmed entry
  auto it = unconfirmed.find({quad, proto});

  // if not found
  if(it == unconfirmed.end())
  {
    // create the entry
    entry = std::make_shared<Entry>(quad, proto);

    // insert it into the map of unconfirmed entries
    unconfirmed.emplace(std::piecewise_construct,
      std::forward_as_tuple(entry->first, proto),
      std::forward_as_tuple(entry));
    // think it's enough to only store one way.
    printf("<Conntrack> Entry added: %s\n", entry->to_string().c_str());
  }
  else {
    entry = it->second;
    printf("<Conntrack> Entry already seen: %s\n", entry->to_string().c_str());
  }

  update_timeout(*entry, std::chrono::seconds(10));

  return entry.get();
}

void Conntrack::update_entry(
  const Protocol proto, const Quadruple& oldq, const Quadruple& newq)
{
  // find the entry that has quintuple containing the old quant
  const auto quint = Quintuple{oldq, proto};
  auto it = entries.find(quint);
  auto entry = it->second;

  // determine if the old quant hits the first or second quantuple
  auto& quad = (entry->first == oldq)
    ? entry->first : entry->second;

  // give it a new value
  quad = newq;

  // TODO: this could probably be optimized with C++17 map::extract
  // erase the old entry
  entries.erase(quint);
  // insert the entry with updated quintuple
  entries.emplace(std::piecewise_construct,
    std::forward_as_tuple(newq, proto),
    std::forward_as_tuple(entry));

  printf("<Conntrack> Entry updated: %s\n", entry->to_string().c_str());
}

void Conntrack::flush_expired()
{
  const auto NOW = RTC::now();
  // unconfirmed data structure
  {
    auto it = unconfirmed.begin();
    while(it != unconfirmed.end())
    {
      auto tmp = it++;

      if(tmp->second->timeout > NOW)
        continue;

      if(tmp->second.unique() && on_close)
        on_close(tmp->second.get());

      unconfirmed.erase(tmp);
    }
  }
  // entries data structure
  {
    auto it = entries.begin();
    while(it != entries.end())
    {
      auto tmp = it++;
      if(tmp->second->timeout > NOW)
        continue;

      if(tmp->second.unique() && on_close)
        on_close(tmp->second.get());

      entries.erase(tmp);
    }
  }
}

void Conntrack::remove_entry(Entry* entry)
{
  // TODO: Mega dangerous, destroying storage to the entry pointer
  entries.erase({entry->first, entry->proto});
  entries.erase({entry->second, entry->proto});

  if(on_close) on_close(entry);
}

void Conntrack::update_timeout(Entry& ent, Timeout_duration dur)
{
  ent.timeout = RTC::now() + dur.count();
  printf("<Conntrack> Timeout updated with %llu secs\n", dur.count());
}


}
