
#pragma once
#ifndef NET_BUFFER_STORE_HPP
#define NET_BUFFER_STORE_HPP

#include <common>
#include <stdexcept>
#include <vector>
#include <smp>

namespace net
{
  /**
   * Network buffer storage for uniformly sized buffers.
   *
   * @note : The buffer store is intended to be used by Packet, which is
   * a semi-intelligent buffer wrapper, used throughout the IP-stack.
   *
   * There shouldn't be any need for raw buffers in services.
   **/
  class BufferStore {
  public:
    BufferStore(uint32_t num, uint32_t bufsize);
    ~BufferStore();

    uint8_t* get_buffer();

    inline void release(void*);

    /** Get size of a buffer **/
    uint32_t bufsize() const noexcept
    { return bufsize_; }

    uint32_t poolsize() const noexcept
    { return poolsize_; }

    /** Check if an address belongs to this buffer store */
    bool is_valid(uint8_t* addr) const noexcept
    {
      for (const auto* pool : pools_)
          if ((addr - pool) % bufsize_ == 0
            && addr >= pool && addr < pool + poolsize_) return true;
      return false;
    }

    size_t available() const noexcept {
      return this->available_.size();
    }

    size_t total_buffers() const noexcept {
      return this->pool_buffers() * this->pools_.size();
    }

    size_t buffers_in_use() const noexcept {
      return this->total_buffers() - this->available();
    }

    /** move this bufferstore to the current CPU **/
    void move_to_this_cpu() noexcept;

  private:
    uint32_t pool_buffers() const noexcept { return poolsize_ / bufsize_; }
    void create_new_pool();
    bool growth_enabled() const;

    uint32_t              poolsize_;
    uint32_t              bufsize_;
    int                   index = -1;
    std::vector<uint8_t*> available_;
    std::vector<uint8_t*> pools_;
    // has strict alignment reqs, so put at end
    smp_spinlock          plock;
    BufferStore(BufferStore&)  = delete;
    BufferStore(BufferStore&&) = delete;
    BufferStore& operator=(BufferStore&)  = delete;
    BufferStore  operator=(BufferStore&&) = delete;
  };

  inline void BufferStore::release(void* addr)
  {
    auto* buff = (uint8_t*) addr;
    if (LIKELY(this->is_valid(buff))) {
      plock.lock();
      this->available_.push_back(buff);
      plock.unlock();
      return;
    }
    throw std::runtime_error("Buffer did not belong");
  }

} //< net

#endif //< NET_BUFFER_STORE_HPP
