#ifndef CLASS_PACKET_H
#define CLASS_PACKET_H

#include <net/ip4.hpp>

namespace net
{
  class Packet {
  public:
    
    /** Get the buffer */
    uint8_t* buffer() const
    { return _data; }
    
    /** Get the network packet length - i.e. the number of populated bytes  */
    inline uint32_t len() const
    { return _len; }
    
    /** Get the size of the buffer. This is >= len(), usually MTU-size */
    inline uint32_t bufsize() const
    { return _bufsize; }
    
    int set_len(uint32_t l);
    
    //! Returns the remaining bytes we can fill into this packet,
    //! before the packet is completely full
    inline uint32_t capacity() const
    {
      return bufsize() - len();
    }
    //! Returns true if the packets internal buffer is full
    bool full() const
    {
      return capacity() == 0;
    }
    
    /** Status of the buffer.
        AVAILABLE : It's just sitting there, free for use
        UPSTREAM : travelling upstream
        DOWNSTREAM : travelling downstream          */
    enum packet_status{ AVAILABLE, UPSTREAM, DOWNSTREAM };
    
    /** Get the packet status */
    packet_status status();
    
    /** Set next-hop ip4. */
    void next_hop(IP4::addr ip);
    
    /** Get next-hop ip4. */
    IP4::addr next_hop();
    
    /** Construct, using existing buffer. */
    Packet(uint8_t* data, uint32_t len, packet_status stat);

    /** Construct, allocating new buffer. */
    //Packet(uint32_t len);
    
    
    /** Destruct. */
    virtual ~Packet();
    
    // for a UPDv6 packet, the payload location is
    // the start of the UDPv6 header, and so on
    inline void set_payload(uint8_t* location)
    {
      this->_payload = location;
    }
    inline uint8_t* payload() const
    {
      return _payload;
    }
    
    // Upcast back to normal packet
    // unfortunately, we can't upcast with std::static_pointer_cast
    // however, all classes derived from Packet should be good to use
    std::shared_ptr<Packet> packet()
    {
      return *(std::shared_ptr<Packet>*)this;
    }
    
  protected:
    uint8_t* _payload;
    uint8_t* _data;
    uint32_t _len;
    uint32_t _bufsize = 1500;
    packet_status _status;
    IP4::addr _next_hop4;
  };
  
}

#endif
