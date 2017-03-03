#include "readq.hpp"
#include "common.hpp"
#include <common>

bool ReadQ::read(uint8_t* buf, size_t len, on_read_func on_read)
{
  while (len > 0) {
    
    int search = -1;
    
    // find line ending
    for (size_t i = 0; i < len; i++)
    if (buf[i] == 13 || buf[i] == 10) {
      search = i; break;
    }
    
    // not found:
    if (UNLIKELY(search == -1))
    {
      // if clients are sending too much data to server, kill them
      if (UNLIKELY(buffer.size() + len >= READQ_MAX)) {
        return false;
      }
      // append entire buffer
      this->buffer.append((const char*) buf, len);
      break;
    }
    else if (UNLIKELY(search == 0)) {
      buf++; len--;
    } else {
      
      // found CR LF:
      // if clients are sending too much data to server, kill them
      if (UNLIKELY(buffer.size() + search >= READQ_MAX)) {
        return false;
      }
      // append to clients buffer
      this->buffer.append((const char*) buf, search);
      
      // move forward in socket buffer
      buf += search;
      // decrease len
      len -= search;
      
      // parse message
      if (!buffer.empty())
      {
        on_read(this->buffer);
        this->buffer.clear();
      }
      
      // skip over continous line ending characters
      if (len != 0 && (buf[0] == 13 || buf[0] == 10)) {
          buf++; len--;
      }
    }
  }
  return true;
}
