#ifndef BUFFER_STORE_HPP
#define BUFFER_STORE_HPP

#include <deque>
#include <stdexcept>
#include <net/inet_common.hpp>

namespace net{
    
  /** Network buffer storage for uniformely sized buffers. 
      @note : The buffer store is intended to be used by Packet, which is a semi-intelligent buffer wrapper, used throughout the IP-stack. There shouldn't be any need for raw buffers in services.  **/
  class BufferStore {
    size_t bufcount_ = INITIAL_BUFCOUNT;
    const size_t bufsize_ = MTUSIZE;
    size_t device_offset_ = 0;
    net::buffer pool_ = 0;
    std::deque<buffer> available_buffers_; 
    
  public:
    
    using release_del = delegate<void(net::buffer, size_t)>;
    
    BufferStore(BufferStore& cpy) = delete;
    BufferStore(BufferStore&& cpy) = delete;
    BufferStore& operator=(BufferStore& cpy) = delete;
    BufferStore operator=(BufferStore&& mv) = delete;
    BufferStore() = delete;
    
    inline size_t raw_bufsize(){ return bufsize_; }
    inline size_t offset_bufsize(){ return bufsize_ - device_offset_; }
    
    /** Free all the buffers **/
    ~BufferStore();
    
    BufferStore(int num, size_t bufsize, size_t device_offset);
    
    /** Get a free buffer */    
    buffer get_raw_buffer();
    
    /** Get a free buffer, offset by device-offset */ 
    buffer get_offset_buffer();    
  
    /** Return a buffer. */
    void release_raw_buffer(buffer b, size_t);
    
    /** Return a buffer, offset by offset_ bytes from actual buffer. */
    void release_offset_buffer(buffer b, size_t);
    
    /** @return the total buffer capacity in bytes */
    inline size_t capacity(){
      return available_buffers_.size() * bufsize_;
    }
    
    /** Check if a buffer belongs here */
    inline bool address_is_from_pool(buffer addr)
    { return addr >= pool_ and addr < pool_ + (bufcount_ * bufsize_); }
    
    /** Check if an address is the start of a buffer */
    inline bool address_is_bufstart(buffer addr)
    { return (addr - pool_) % bufsize_ == 0; }
    
    /** Check if an address is the start of a buffer */
    inline bool address_is_offset_bufstart(buffer addr)
    { return (addr - pool_ - device_offset_) % bufsize_ == 0; }

    
  private:
    
    void increaseStorage();

    
  };
  

}

#endif
