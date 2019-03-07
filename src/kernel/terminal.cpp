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

#include <net/ip4/ip4.hpp>
#include <net/ip6/ip6.hpp>
#include <net/inet>
#include <kernel/terminal.hpp>
#include <os.hpp>
#include <cstdio>
#include <deque>

static std::unordered_map<std::string, TerminalProgram> command_registry;
void Terminal::register_program(std::string name, TerminalProgram program)
{
  command_registry.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(name),
          std::forward_as_tuple(program.desc, std::move(program.main)));
}

Terminal::Terminal(Connection_ptr csock)
  : stream(std::move(csock))
{
  register_basic_commands();

  stream->on_read(1024, [this](auto buffer) {
    this->read((char*) buffer->data(), buffer->size());
  });

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
      exec(buffer);
      buffer.clear();
      // show prompt again
      prompt();
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
    x = text.find(' ');
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
    x = text.find(' ', p+1);
    size_t y = text.find(':', x+1); // find last param

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

int Terminal::exec(const std::string& cmd_string)
{
  std::string cmd_name;
  auto cmd_vec = split(cmd_string, cmd_name);
  if (cmd_name.empty()) return -1;

  auto it = command_registry.find(cmd_name);
  if (it != command_registry.end()) {
    int retv = it->second.main(*this, cmd_vec);
    if (retv) write("%s returned: %d\r\n", cmd_name.c_str(), retv);
    return retv;
  }
  write("No such command: '%s'\r\n", cmd_name.c_str());
  return -1;
}

void Terminal::close()
{
  stream->close();
}

static int send_help(Terminal& term, const std::vector<std::string>&)
{
  for (auto cmd : command_registry) {
    term.write("%s \t-- %s\r\n", cmd.first.c_str(), cmd.second.desc.c_str());
  }
  return 0;
}

void Terminal::register_basic_commands()
{
  register_program("?", {"List available commands", send_help});
  register_program("help", {"List available commands", send_help});
  // exit:
  register_program("exit", {"Close the terminal",
  [] (Terminal& term, const std::vector<std::string>&) -> int
  {
    term.close();
    return 0;
  }});

}
void Terminal::intro()
{
  const std::string banana =
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

> Banana Terminal v2 <
)baaa";

  stream->write(banana);
  prompt();
}

void Terminal::prompt()
{
  stream->write("$ ");
}
