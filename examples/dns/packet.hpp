#ifndef PACKET_HPP
#define PACKET_HPP

#include <stdint.h>

class Packet {

 public:

  /** Get the buffer */
  uint8_t* buffer() const { return _data; }

  /** Get the buffer length */
  inline uint32_t len() const { return _len; }

  /** Status of the buffer.
      AVAILABLE : It's just sitting there, free for use
      UPSTREAM : travelling upstream
      DOWNSTREAM : travelling downstream */
  enum packet_status{ AVAILABLE, UPSTREAM, DOWNSTREAM };
  /** Get the packet status */
  //packet_status status();
  /** Set next-hop ip4. */
  //void next_hop(IP4::addr ip);
  /** Get next-hop ip4. */
  //IP4::addr next_hop();

  /** Construct. */
  Packet(uint8_t* data, uint32_t len, packet_status stat);
  /** Destruct. */
  ~Packet();

 private:
  uint8_t* _data;
  uint32_t _len;
  packet_status _status;
  //IP4::addr _next_hop4;
  uint32_t _next_hop4;
};

#endif
