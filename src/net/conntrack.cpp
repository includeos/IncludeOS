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
#include <set>

//#define CT_DEBUG 1
#ifdef CT_DEBUG
#define CTDBG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define CTDBG(fmt, ...) /* fmt */
#endif

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

std::string state_str(const Conntrack::State state)
{
  switch(state) {
    case Conntrack::State::NEW: return "NEW";
    case Conntrack::State::ESTABLISHED: return "EST";
    case Conntrack::State::RELATED: return "RELATED";
    case Conntrack::State::UNCONFIRMED: return "UNCONFIRMED";
    default: return "???";
  }
}

std::string flag_str(const uint8_t flags)
{
  std::string str;
  if(flags & static_cast<uint8_t>(Conntrack::Flag::UNREPLIED))
    str.append(" UNREPLIED");
  if(flags & static_cast<uint8_t>(Conntrack::Flag::ASSURED))
    str.append(" ASSURED");
  return str;
}

std::string Conntrack::Entry::to_string() const
{
  return "[ " + first.to_string() + " ] [ " + second.to_string() + " ]"
    + " P: " + proto_str(proto) + " S: " + state_str(state) + " F:" + flag_str(flags);
}

Conntrack::Entry::~Entry()
{
  if(this->on_close)
    on_close(this);
}

Conntrack::Entry* Conntrack::simple_track_in(Quadruple q, const Protocol proto)
{
  // find the entry
  auto* entry = get(q, proto);

  CTDBG("<Conntrack> Track in S: %s - D: %s\n",
    q.src.to_string().c_str(), q.dst.to_string().c_str());

  // if none, add new and return
  if(entry == nullptr)
  {
    entry = add_entry(q, proto);
    return entry;
  }

  // temp
  CTDBG("<Conntrack> Entry found: %s\n", entry->to_string().c_str());

  if(entry->state == State::NEW and q == entry->second)
  {
    entry->state = State::ESTABLISHED;
    CTDBG("<Conntrack> Assuming ESTABLISHED\n");
  }

  update_timeout(*entry, (entry->state == State::ESTABLISHED) ? timeout.established : timeout.confirmed);

  return entry;
}

Conntrack::Entry* dumb_in(Conntrack& ct, Quadruple q, const PacketIP4& pkt)
{ return  ct.simple_track_in(std::move(q), pkt.ip_protocol()); }

Conntrack::Conntrack()
 : Conntrack(0)
{}

Conntrack::Conntrack(size_t max_entries)
 : maximum_entries{max_entries},
   tcp_in{&dumb_in},
   flush_timer({this, &Conntrack::on_timeout})
{
}

Conntrack::Entry* Conntrack::get(const PacketIP4& pkt) const
{
  const auto proto = pkt.ip_protocol();
  switch(proto)
  {
    case Protocol::TCP:
    case Protocol::UDP:
      return get(get_quadruple(pkt), proto);

    case Protocol::ICMPv4:
      return get(get_quadruple_icmp(pkt), proto);

    default:
      return nullptr;
  }
}

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
  auto id = reinterpret_cast<const partial_header*>(pkt.ip_data().data())->id;

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

  return confirm(quad, proto);
}

Conntrack::Entry* Conntrack::confirm(Quadruple quad, const Protocol proto)
{
  auto* entry = get(quad, proto);

  if(UNLIKELY(entry == nullptr)) {
    CTDBG("<Conntrack> Entry not found on confirm, checking swapped: %s\n",
      quad.to_string().c_str());
    // the packet my be NATed. note: not sure if this is good
    if(UNLIKELY((entry = get(quad.swap(), proto)) == nullptr)) {
      return nullptr;
    }
  }

  if(entry->state == State::UNCONFIRMED)
  {
    CTDBG("<Conntrack> Confirming %s\n", entry->to_string().c_str());
    entry->state = State::NEW;
    update_timeout(*entry, timeout.confirmed);
  }

  return entry;
}

Conntrack::Entry* Conntrack::add_entry(
  const Quadruple& quad, const Protocol proto)
{
  // Return nullptr if conntrack is full
  if(UNLIKELY(maximum_entries != 0 and
    entries.size() + 2 > maximum_entries))
  {
    CTDBG("<Conntrack> Limit reached (limit=%lu sz=%lu)\n",
      maximum_entries, entries.size());
    return nullptr;
  }

  if(not flush_timer.is_running())
    flush_timer.start(flush_interval);

  // we dont check if it's already exists
  // because it should be called from in()

  // create the entry
  auto entry = std::make_shared<Entry>(quad, proto);

  entries.emplace(std::piecewise_construct,
    std::forward_as_tuple(entry->first, proto),
    std::forward_as_tuple(entry));

  entries.emplace(std::piecewise_construct,
    std::forward_as_tuple(entry->second, proto),
    std::forward_as_tuple(entry));

  CTDBG("<Conntrack> Entry added: %s\n", entry->to_string().c_str());

  update_timeout(*entry, timeout.unconfirmed);

  return entry.get();
}

Conntrack::Entry* Conntrack::update_entry(
  const Protocol proto, const Quadruple& oldq, const Quadruple& newq)
{
  // find the entry that has quintuple containing the old quant
  const auto quint = Quintuple{oldq, proto};
  auto it = entries.find(quint);

  if(UNLIKELY(it == entries.end())) {
    CTDBG("<Conntrack> Cannot find entry when updating: %s\n",
      oldq.to_string().c_str());
    return nullptr;
  }

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

  CTDBG("<Conntrack> Entry updated: %s\n", entry->to_string().c_str());

  return entry.get();
}

void Conntrack::remove_expired()
{
  CTDBG("<Conntrack> Removing expired entries\n");
  const auto NOW = RTC::now();
  // entries data structure
  for(auto it = entries.begin(); it != entries.end();)
  {
    if(it->second->timeout > NOW) {
      ++it;
    }
    else {
      CTDBG("<Conntrack> Erasing %s\n", it->second->to_string().c_str());
      it = entries.erase(it);
    }
  }
}

void Conntrack::on_timeout()
{
  remove_expired();

  if(not entries.empty())
    flush_timer.restart(flush_interval);
}

int Conntrack::Entry::deserialize_from(void* addr)
{
  auto& entry = *reinterpret_cast<Entry*>(addr);
  this->first   = entry.first;
  this->second  = entry.second;
  this->timeout = entry.timeout;
  this->proto   = entry.proto;
  this->state   = entry.state;
  return sizeof(Entry) - sizeof(on_close);
}

void Conntrack::Entry::serialize_to(std::vector<char>& buf) const
{
  const size_t size = sizeof(Entry) - sizeof(on_close);
  const auto* ptr = reinterpret_cast<const char*>(this);
  buf.insert(buf.end(), ptr, ptr + size);
}

int Conntrack::deserialize_from(void* addr)
{
  const auto prev_size = entries.size();
  auto* buffer = reinterpret_cast<uint8_t*>(addr);

  const auto size = *reinterpret_cast<size_t*>(buffer);
  buffer += sizeof(size_t);

  for(auto i = size; i > 0; i--)
  {
    // create the entry
    auto entry = std::make_shared<Entry>();
    buffer += entry->deserialize_from(buffer);

    entries.emplace(std::piecewise_construct,
      std::forward_as_tuple(entry->first, entry->proto),
      std::forward_as_tuple(entry));

    entries.emplace(std::piecewise_construct,
      std::forward_as_tuple(entry->second, entry->proto),
      std::forward_as_tuple(entry));
  }

  Ensures(entries.size() - prev_size == size * 2);

  return buffer - reinterpret_cast<uint8_t*>(addr);
}

void Conntrack::serialize_to(std::vector<char>& buf) const
{
  int unserialized = 0;

  // Since each entry is stored twice in the map,
  // we iterate and put it in a set if not already there
  std::set<Entry*> to_serialize;
  for(auto& i : entries)
  {
    auto* ent = i.second.get();

    // We cannot restore delegates, so just ignore
    // the ones with close handler set
    if(ent->on_close != nullptr) {
      unserialized++;
      continue;
    }
    // If not in set, add
    if(to_serialize.find(ent) == to_serialize.end())
      to_serialize.emplace(ent);
  }

  // Serialize number of entries
  size_t size = to_serialize.size();
  const auto* size_ptr = reinterpret_cast<const char*>(&size);

  const auto expected_buf_size = sizeof(size) + (size * (sizeof(Entry) - sizeof(Entry_handler)));
  buf.reserve(expected_buf_size);

  buf.insert(buf.end(), size_ptr, size_ptr + sizeof(size));
  // Serialize each entry
  for(auto& ent : to_serialize)
    ent->serialize_to(buf);

  if(unserialized > 0)
    INFO("Conntrack", "%i entries not serialized\n", unserialized);
}


}
