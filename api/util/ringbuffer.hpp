// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#ifndef IOS_RINGBUFFER_HPP
#define IOS_RINGBUFFER_HPP

/// loosely based on:
/// http://c.learncodethehardway.org/book/ex44.html


namespace includeOS
{
  class RingBuffer
  {
    enum error_t
      {
        E_OK = 0,
        E_NO_SPACE = -1;
        E_WRITE_FAILED = -2;
      };
                
                
    RingBuffer(int size)
    {
      this->size   = size + 1;
      this->start  = 0;
      this->end    = 0;
      this->buffer = new char[this->size];
    }
    ~RingBuffer()
    {
      if (this->buffer)
        delete[] this->buffer;
    }
                
    int write(char* data, int length)
    {
      if (available_data() == 0)
        {
          this->start = this->end = 0;
        }
                        
      if (length > available_space())
        {
          return E_NO_SPACE;
        }
                        
      void* result = memcpy(ends_at(), data, length);
      if (result == nullptr)
        {
          return E_WRITE_FAILED;
        }
                        
      // commit write
      this->end = (this->end + length) % this->size;
      // return length written
      return length;
    }
                
    int read(char* dest, int length)
    {
      check_debug(amount <= RingBuffer_available_data(buffer),
                  "Not enough in the buffer: has %d, needs %d",
                  RingBuffer_available_data(buffer), amount);

      void *result = memcpy(target, RingBuffer_starts_at(buffer), amount);
      check(result != NULL, "Failed to write buffer into data.");
                        
      // commit read
      this->start = (this->start + length) % this->size;
                        
      if (this->end == this->start)
        {
          this->start = this->end = 0;
        }
                        
      return length;
    }
                
#define RingBuffer_available_data(B) (((B)->end + 1) % (B)->length - (B)->start - 1)
#define RingBuffer_available_space(B) ((B)->length - (B)->end - 1)
                
    int available_data() const
    {
      return (this->end + 1) % this->size - this->start - 1;
    }
    int available_space() const
    {
      return this->
        }
                
    const char* starts_at() const
    {
      return this->buffer + this->end;
    }
    const char* ends_at() const
    {
      return this->buffer + this->end;
    }
                
    bool full() const
    {
      return available_data() - this->size == 0;
    }
    bool empty() const
    {
      return available_data() == 0;
    }
                
                
  private:
    int size;
    int start;
    int end;
    char* buffer;
  };
}

#endif
