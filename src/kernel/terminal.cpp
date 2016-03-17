#include <kernel/terminal.hpp>

#include <vector>
#include <serial>
#include <cstdio>

Terminal::Terminal(Connection_ptr csock)
  : iac(false)
{
  csock->onReceive(
  [this] (auto conn, bool)
  {
    char buffer[1024];
    size_t bytes = conn->read(buffer, sizeof(buffer));
    
    this->read(buffer, bytes);
  });
  
  on_write = 
  [csock] (const char* buffer, size_t len)
  {
    csock->write(buffer, len);
  };
}

void Terminal::read(const char* buf, size_t len)
{
  while (len)
  {
    if (this->iac)
    {
      command(*(uint8_t*) buf);
      this->iac = false;
    }
    else if (*buf == 13 || *buf == 10)
    {
      // parse message
      if (buffer.size())
      {
        run(buffer);
        buffer.clear();
      }
    }
    else if (*buf == 0)
    {
      // NOP
    }
    else if (*buf == -128)
    {
      // Interpret as Command
      this->iac = true;
    }
    else
    {
      buffer.append(buf, 1);
    }
    buf++; len--;
  }
}

void Terminal::command(uint8_t cmd)
{
  printf("Command: 0x%x\n", cmd);
  switch (cmd)
  {
  case 247: // erase char
      printf("CMD: Erase char\n");
      break;
  case 248: // erase line
      printf("CMD: Erase line\n");
      break;
      
  }
}

std::vector<std::string>
split(const std::string& text, std::string& command)
{
  std::vector<std::string> retv;
  size_t x = 0;
  size_t p = 0;
  // ignore empty messages
  if (text.empty()) return retv;
  // extract command
  {
    x = text.find(" ");
    // early return for cmd-only msg
    if (x == std::string::npos)
    {
      command = text;
      return retv;
    }
    // command is substring
    command = text.substr(x);
    p = x+1;
  }
  // parse remainder
  do
  {
    x = text.find(" ", p+1);
    size_t y = text.find(":", x+1); // find last param
    
    if (y == x+1)
    {
      // single argument
      retv.push_back(text.substr(p, x-p));
      // ending text argument
      retv.push_back(text.substr(y+1));
      break;
    }
    else if (x != std::string::npos)
    {
      // single argument
      retv.push_back(text.substr(p, x-p));
    }
    else
    {
      // last argument
      retv.push_back(text.substr(p));
    }
    p = x+1;
    
  } while (x != std::string::npos);
  
  return retv;
}

void Terminal::run(const std::string& cmd_string)
{
  std::string cmd_name;
  auto cmd_vec = split(cmd_string, cmd_name);
  if (cmd_name.empty()) return;
  
  printf("Terminal::run(): %s\n", cmd_name.c_str());
  
  auto it = commands.find(cmd_name);
  if (it != commands.end())
  {
    int retv = it->second.main(cmd_vec);
    if (!retv) write("%s returned failure\r\n", cmd_name.c_str());
  }
  else
  {
    write("No such command: %s\r\n", cmd_name.c_str());
  }
  
}
