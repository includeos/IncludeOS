#pragma once

#include <vector>
#include <stdexcept>
#include <net/inet_common.hpp>

namespace net{
    
  /** Network buffer storage for uniformely sized buffers. 
      @note : The buffer store is intended to be used by Packet, which is a semi-intelligent buffer wrapper, used throughout the IP-stack. There shouldn't be any need for raw buffers in services.  **/
  class BufferStore {
    const size_t bufsize_ = MTUSIZE;
    buffer pool_;
    size_t bufcount_ = INITIAL_BUFCOUNT;
    std::vector<buffer> available_buffers_;


    
  public:
  
    BufferStore(BufferStore& cpy) = delete;
    BufferStore(BufferStore&& cpy) = delete;
    BufferStore& operator=(BufferStore& cpy) = delete;
    BufferStore operator=(BufferStore&& mv) = delete;
    BufferStore() = delete;

    inline size_t bufsize(){ return bufsize_; }
    
    /** Free all the buffers **/
    ~BufferStore();
  
    BufferStore(int num, size_t bufsize);
  
    /** Get a free buffer */
    buffer get();
  
    /** Return a buffer. */
    void release(buffer b);
    
  private:
    
    void increaseStorage();

    
  };
  

}
