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

#include <kernel/terminal.hpp>
#include <net/inet4>

#include <vector>
#include <serial>
#include <cstdio>

Terminal::Terminal()
  : iac(false), newline(false), subcmd(0)
{
  add_basic_commands();
}

Terminal::Terminal(Connection_ptr csock)
  : Terminal()
{
  csock->on_read(1024, [this](auto buffer, size_t n) {
    this->read((char*)buffer.get(), n);
  });

  on_write =
  [csock] (const char* buffer, size_t len) {
    csock->write(buffer, len);
  };

  on_exit =
  [csock] {
    csock->close();
  };
  // after setting up everything, show introduction
  intro();
}

Terminal::Terminal(hw::Serial& serial)
  : Terminal()
{
  serial.on_data(
  [this, &serial] (char c)
  {
    this->read(&c, 1);
    if (c == CR) {
      c = LF;
      this->read(&c, 1);
    }
    else {
      serial.write(c);
    }
  });

  on_write =
    [&serial] (const char* buffer, size_t len)
    {
      for (size_t i = 0; i < len; i++)
        serial.write(buffer[i]);
    };

  on_exit =
    [] {
    // do nothing
  };
  // after setting up everything, show introduction
  intro();
}

void Terminal::read(const char* buf, size_t len)
{
  while (len) {
    if (this->subcmd) {
      // execute options
      option(this->subcmd, (uint8_t) *buf);
      this->subcmd = 0;
    }
    else if (this->iac) {
      command(*(uint8_t*) buf);
      this->iac = false;
    }
    else if (*buf == 13 && !newline) {
      newline = true;
    }
    else if (*buf == 10 && newline) {
      newline = false;
      // parse message
      run(buffer);
      buffer.clear();
    }
    else if (*buf == 0) {
      // NOP
    }
    else if ((uint8_t) *buf == 0xFF) {
      // Interpret as Command
      this->iac = true;
    }
    else {
      buffer.append(buf, 1);
    }
    buf++; len--;
  }
}

void Terminal::command(uint8_t cmd)
{
  switch (cmd) {
  case 247: // erase char
    printf("CMD: Erase char\n");
    break;
  case 248: // erase line
    printf("CMD: Erase line\n");
    break;
  case 250: // Begin
    printf("CMD: Begin...\n");
    break;
  case 251: // Will USE
  case 252: // Won't USE
  case 253: // Start USE
  case 254: // Stop USE
    //printf("CMD: Command %d\n", cmd);
    this->subcmd = cmd;
    break;
  case 255:
    //printf("CMD: Command %d\n", cmd);
    this->subcmd = cmd;
    break;
  default:
    printf("CMD: Unknown command %d\n", cmd);
  }
}

void Terminal::option(uint8_t option, uint8_t cmd)
{
  (void) option;
  switch (cmd) {
  case 24: // terminal type
    break;
  default:
    //printf("CMD: Unknown cmd %d for option %d\n", cmd, option);
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
    command = text.substr(0, x);
    p = x+1;
  }
  // parse remainder
  do {
    x = text.find(" ", p+1);
    size_t y = text.find(":", x+1); // find last param

    if (y == x+1) {
      // single argument
      retv.push_back(text.substr(p, x-p));
      // ending text argument
      retv.push_back(text.substr(y+1));
      break;
    }
    else if (x != std::string::npos) {
      // single argument
      retv.push_back(text.substr(p, x-p));
    }
    else {
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
  if (cmd_name.size())
  {
    printf("Terminal::run(): %s\n", cmd_name.c_str());

    auto it = commands.find(cmd_name);
    if (it != commands.end()) {
      int retv = it->second.main(cmd_vec);
      if (retv) write("%s returned: %d\r\n", cmd_name.c_str(), retv);
    }
    else {
      write("No such command: '%s'\r\n", cmd_name.c_str());
    }
  }
  prompt();
}

void Terminal::add_basic_commands()
{
  // ?:
  add("?", "List available commands",
  [this] (const std::vector<std::string>&) -> int
  {
    for (auto cmd : this->commands)
    {
      write("%s \t-- %s\r\n", cmd.first.c_str(), cmd.second.desc.c_str());
    }
    return 0;
  });
  // exit:
  add("exit", "Close the terminal",
  [this] (const std::vector<std::string>&) -> int
  {
    this->on_exit();
    return 0;
  });

}
void Terminal::intro()
{
  std::string banana =
    R"baaa(
     ____                           ___
    |  _ \  ___              _   _.' _ `.
 _  | [_) )' _ `._   _  ___ ! \ | | (_) |    _
|:;.|  _ <| (_) | \ | |' _ `|  \| |  _  |  .:;|
|   `.[_) )  _  |  \| | (_) |     | | | |.',..|
':.   `. /| | | |     |  _  | |\  | | |.' :;::'
!::,   `-!_| | | |\  | | | | | \ !_!.'   ':;!
!::;       ":;:!.!.\_!_!_!.!-'-':;:''    '''!
';:'        `::;::;'             ''     .,  .
     `:     .,.    `'    .::... .      .::;::;'
     `..:;::;:..      ::;::;:;:;,    :;::;'
     "-:;::;:;:      ':;::;:''     ;.-'
         ""`---...________...---'""

> Banana Terminal v1 <
)baaa";

  write("%s", banana.c_str());
  prompt();
}

void Terminal::prompt()
{
  write("%s", "$ ");
}
