#ifndef NET_PACKET_HPP
#define NET_PACKET_HPP

#include <net/inet_common.hpp>
#include <net/buffer_store.hpp>
#include <net/ip4.hpp>

namespace net {

  /** Default buffer release-function. Returns the buffer to Packet's bufferStore  **/
  void default_release(net::buffer, size_t);
  
  
  class Packet {
  public:
    
    using release_del = BufferStore::release_del;
    
    /** Get the buffer */
    net::buffer buffer() const
    { return buf_; }
    
    /** Get the network packet length - i.e. the number of populated bytes  */
    inline uint32_t size() const
    { return size_; }
    
    /** Get the size of the buffer. This is >= len(), usually MTU-size */
    inline uint32_t capacity() const
    { return capacity_; }
    
    int set_size(size_t);
    
    /** Set next-hop ip4. */
    void next_hop(IP4::addr ip);
    
    /** Get next-hop ip4. */
    IP4::addr next_hop();
    
    /** Construct, using existing buffer.
	@param buf : The buffer
	@param bufsize : size of the buffer
	@param datalen : Length of data in the buffer
	@WARNING : There are two adjacent parameters of the same type, violating CG I.24. 
     */    
    Packet(const net::buffer buf, size_t bufsize, size_t datalen, release_del d = default_release);
    
    /** Destruct. */
    virtual ~Packet();
            
    /** Copy constructor. 
	Deleted because we want Packets and buffers to be 1 to 1. 
	(Well, we really deleted this to avoid accidental copying)
	The idea is to use Packet_ptr (i.e. shared_ptr<Packet>) for passing packets.
	@todo Add an explicit way to copy packets. 
     */
    Packet(Packet&) = delete;
    
    /** Move constructor.  Deleted. See Packet(Packet&). */
    Packet(Packet&&) = delete;

    /** Default constructor Deleted. See Packet(Packet&). */
    Packet() = delete;
    
    /** Copy assignment operator Deleted. See Packet(Packet&). */
    Packet& operator=(Packet) = delete;
    
    /** Move assignment operator Deleted. See Packet(Packet&). */
    Packet operator=(Packet&&) = delete;
        

    // for a UPDv6 packet, the payload location is
    // the start of the UDPv6 header, and so on
    inline void set_payload(uint8_t* location)
    {
      this->payload_ = location;
    }
    
    inline uint8_t* payload() const
    {
      return payload_;
    }
    
    // transformed back to normal packet
    // unfortunately, we can't downcast with std::static_pointer_cast
    // however, all classes derived from Packet should be good to use
    std::shared_ptr<Packet> packet()
    {
      return *(std::shared_ptr<Packet>*)this;
    }


    /** @todo Avoid Protected Data. (Jedi Council CG, C.133) **/
  protected:
    uint8_t* payload_ = nullptr;
    net::buffer buf_ = 0;
    uint32_t capacity_ = MTUSIZE;
    uint32_t size_ = 0;
    IP4::addr next_hop4_ {};
    
  private:
    
    /** Send the buffer back home, after destruction */
    release_del release_ = default_release;
    
  };
  
}


#endif
