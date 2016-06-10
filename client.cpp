#include "client.hpp"

#include "ircsplit.hpp"
#include "ircd.hpp"

void Client::split_message(const std::string& msg)
{
  std::string source;
  auto vec = split(msg, source);
  
  printf("[Client]: ");
  for (auto& str : vec)
  {
    printf("[%s]", str.c_str());
  }
  printf("\n");
  // ignore empty messages
  if (vec.size() == 0) return;
  // handle message
  if (this->is_reg() == false)
    handle_new(source, vec);
  else
    handle(source, vec);
}

void Client::read(const uint8_t* buf, size_t len)
{
  while (len > 0) {
    
    int search = -1;
    
    // find line ending
    for (size_t i = 0; i < len; i++)
    if (buf[i] == 13 || buf[i] == 10) {
      search = i; break;
    }
    
    // not found:
    if (search == -1)
    {
      // append entire buffer
      buffer.append((char*) buf, len);
      break;
    }
    else {
      // found CR LF:
      if (search != 0) {
        // append to clients buffer
        buffer.append((char*) buf, search);
  
        // move forward in socket buffer
        buf += search;
        // decrease len
        len -= search;
      }
      else {
        buf++; len--;
      }
  
      // parse message
      if (buffer.size())
      {
        split_message(buffer);
        buffer.clear();
      }
    }
  }
}

void Client::send(uint16_t numeric, std::string text)
{
  std::string num;
  num.reserve(128);
  num = std::to_string(numeric);
  num = std::string(3 - num.size(), '0') + num;
  
  num = ":" + server.name() + " " + num + " " + this->nick + " " + text + "\r\n";
  //printf("-> %s", num.c_str());
  conn->write(num.c_str(), num.size());
}
void Client::send(std::string text)
{
  std::string data = ":" + server.name() + " " + text + "\r\n";
  //printf("-> %s", data.c_str());
  conn->write(data.c_str(), data.size());
}
