#include <net/ip4/ip4.hpp>
#include <cassert>
#include <rtc>

//#define REASSEMBLY_DEBUG 1
#ifdef REASSEMBLY_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif


namespace net
{
  static const int REASSEMBLY_ENTRIES = 64;
  static const int TIMEOUT = 15;
  //static const int TTL_MAX = 163;
  static const int IP_ALIGN = 2;
  static const int MAX_DATAGRAM = 65515;

  inline net::Packet_ptr create_packet(uint16_t length)
  {
    size_t buffer_len = sizeof(Packet) + IP_ALIGN + length;
    auto  buffer = new uint8_t[buffer_len];
    auto* ptr    = (net::Packet*) buffer;

    new (ptr) net::Packet(IP_ALIGN, 0, IP_ALIGN + length, nullptr);
    return net::Packet_ptr(ptr);
  }

  struct Entry
  {
    ip4::Addr src = IP4::ADDR_ANY;
    Protocol proto;
    uint16_t frag_id;
    DSCP     dscp;
    uint32_t received;
    uint32_t total_length;
    RTC::timestamp_t last_seen;
    IP4::IP_packet_ptr buffer = nullptr;

    Entry() = default;
    inline Entry(RTC::timestamp_t, IP4::IP_packet&);

    bool is_dead(RTC::timestamp_t now) const noexcept {
      return src == IP4::ADDR_ANY || now - last_seen >= TIMEOUT;
    }
    bool matches(RTC::timestamp_t now, IP4::IP_packet& packet) const noexcept {
      return is_dead(now) == false
          && this->src == packet.ip_src()
          && this->proto == packet.ip_protocol()
          && this->frag_id == packet.ip_id();
    }
    void disable() { this->src = IP4::ADDR_ANY; }

    IP4::IP_packet_ptr assemble_from(IP4::IP_packet&);

    auto release() {
      this->src = IP4::ADDR_ANY;
      return std::move(buffer);
    }
  };
  IP4::IP_packet_ptr Entry::assemble_from(IP4::IP_packet& packet)
  {
    const int fragoff = packet.ip_frag_offs() * 8;
    assert(fragoff >= 0);
    // we don't allow non-initial fragments to start with
    if (fragoff != 0 && this->buffer == nullptr) {
      PRINT("-> Non-initial fragment in new buffer, dropping entry\n");
      this->disable();
      return nullptr;
    }

    if (fragoff == 0)
    {
      PRINT("Creating entry buffer\n");
      // first fragment
      auto raw = create_packet(MAX_DATAGRAM);
      this->buffer = static_unique_ptr_cast<IP4::IP_packet> (std::move(raw));
      // set header and options
      buffer->set_ip_version(4);
      buffer->set_ip_header_length(20);
      buffer->set_ip_dscp(packet.ip_dscp());
      buffer->set_ip_ecn(packet.ip_ecn());
      buffer->set_ip_total_length(20);

      buffer->set_ip_ttl(packet.ip_ttl());
      buffer->set_protocol(packet.ip_protocol());
      buffer->set_ip_checksum(0);

      buffer->set_ip_src(packet.ip_src());
      buffer->set_ip_dst(packet.ip_dst());
    }
    else
    {
      // verify member fields match
      if (UNLIKELY(this->dscp != packet.ip_dscp())) {
        PRINT("-> DSCP mismatch in entry, dropping entry\n");
        this->disable();
        return nullptr;
      }
    }
    // validate length
    const int len = packet.ip_data_length();
    if (this->received + len > MAX_DATAGRAM) {
      PRINT("-> data overflow in entry (%u > %u), dropping entry\n",
            this->received + len, MAX_DATAGRAM);
      this->disable();
      return nullptr;
    }
    // add data, increase bytes received
    auto* data = buffer->layer_begin() + buffer->ip_header_length();
    auto* src_data = packet.layer_begin() + packet.ip_header_length();
    std::memcpy(&data[fragoff], src_data, len);
    this->received += len;

    // when last fragment received, record total length
    if (packet.ip_flags() != ip4::Flags::MF) {
      this->total_length = fragoff + len;
      PRINT("-> last fragment received, total length is %u\n",
            this->total_length);
    }
    // if we've received all bytes,
    if (this->total_length != 0)
    {
      // if exact, we're done
      if (this->received == this->total_length)
      {
        buffer->set_ip_total_length(buffer->size());
        buffer->set_ip_data_length(this->total_length);
        PRINT("Shipping large packet (%u / %u)\n",
              buffer->size(), buffer->bufsize());
        return this->release();
      }
      else if (this->received > this->total_length)
      {
        // received too many bytes, possibly overlapping
        PRINT("-> data overflow (%u > %u), dropping entry\n",
              this->received, this->total_length);
        this->disable();
        return nullptr;
      }
    }
    // otherwise, return nothing
    return nullptr;
  }

  Entry::Entry(RTC::timestamp_t now, IP4::IP_packet& packet)
    : frag_id(packet.ip_id()), received(0), total_length(0),
      last_seen(now), buffer(nullptr)
  {
    this->src   = packet.ip_src();
    this->proto = packet.ip_protocol();
    this->dscp  = packet.ip_dscp();
  }

  struct Reassembly
  {
    Reassembly() = default;
    std::array<Entry, REASSEMBLY_ENTRIES> entries {};

    int get_entry(RTC::timestamp_t, IP4::IP_packet&);

    IP4::IP_packet_ptr process(IP4::IP_packet_ptr packet);
  };
  static std::vector<std::pair<IP4&, Reassembly>> assemblies;

  static inline auto& get_assembly(IP4& current)
  {
    for (auto& stk : assemblies) {
      if (&stk.first == &current) return stk.second;
    }
    // create new
    assemblies.emplace_back(current, Reassembly{});
    return assemblies.back().second;
  }

  IP4::IP_packet_ptr IP4::reassemble(IP4::IP_packet_ptr packet)
  {
    assert(packet != nullptr);
    auto& assembly = get_assembly(*this);
    return assembly.process(std::move(packet));
  }

  int Reassembly::get_entry(RTC::timestamp_t now, IP4::IP_packet& packet)
  {
    // first fragment
    if (packet.ip_frag_offs() == 0)
    {
      // assign new entry
      for (int i = 0; i < (int) entries.size(); i++) {
        if (entries[i].is_dead(now)) {
          entries[i] = Entry(now, packet);
          return i;
        }
      }
      // no free entries
      return -1;
    }
    // find matching entry
    for (int i = 0; i < (int) entries.size(); i++)
    {
      if (entries[i].matches(now, packet)) return i;
    }
    // no match, and we don't allow initial non-zero offsets
    return -1;
  }

  IP4::IP_packet_ptr Reassembly::process(IP4::IP_packet_ptr packet)
  {
    // some basic validation
    if (UNLIKELY(packet->ip_data_length() == 0)) return nullptr;
    if (UNLIKELY(packet->ip_src() == IP4::ADDR_ANY)) return nullptr;
    // non-last fragments ...
    if (packet->ip_flags() == ip4::Flags::MF)
    {
      // must have length mult of 8
      if (UNLIKELY(packet->ip_data_length() % 8 != 0)) return nullptr;
      // should be at least 400 octets long
      if (UNLIKELY(packet->ip_data_length() < 400)) return nullptr;
    }
    // create timestamp used for timeouts
    RTC::timestamp_t now = RTC::now();
    // get entry for this fragment
    int idx = this->get_entry(now, *packet);
    if (idx < 0) return nullptr;
    PRINT("Reassembly on %s  id=%u (entry %d)\n",
          packet->ip_src().to_string().c_str(), packet->ip_id(), idx);

    auto& entry = this->entries.at(idx);
    return entry.assemble_from(*packet);
  }
}
