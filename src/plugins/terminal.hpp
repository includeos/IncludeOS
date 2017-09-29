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

#pragma once
#ifndef TERMINAL_PLUGIN_HPP
#define TERMINAL_PLUGIN_HPP

#include <net/stream.hpp>
#include <cstdint>
#include <unordered_map>
#include <vector>

struct Command
{
  using main_func = delegate<int(const std::vector<std::string>&)>;

  Command(const std::string& descr, main_func func)
    : desc(descr), main(func) {}

  std::string desc;
  main_func   main;
};

class Terminal
{
public:
  using Connection_ptr = std::shared_ptr<net::Stream_ptr>;
  enum {
    NUL  = 0,
    BELL = 7,
    BS   = 8,
    HTAB = 9,
    LF   = 10,
    VTAB = 11,
    FF   = 12,
    CR   = 13
  };

  static void initialize();
  Terminal(net::Stream_ptr);

  template <typename... Args>
  void add(const std::string& command,
           Args&&... args)
  {
    commands.emplace(std::piecewise_construct,
                     std::forward_as_tuple(command),
                     std::forward_as_tuple(args...));
  }

  template <typename... Args>
  void write(const char* str, Args&&... args)
  {
    char buffer[1024];
    int bytes = snprintf(buffer, 1024, str, args...);

    stream->write(buffer, bytes);
  }

private:
  void command(uint8_t cmd);
  void option(uint8_t option, uint8_t cmd);
  void read(const char* buf, size_t len);
  void run(const std::string& cmd);
  void add_basic_commands();
  void intro();
  void prompt();

  net::Stream_ptr stream;
  bool    iac     = false;
  bool    newline = false;
  uint8_t subcmd  = 0;
  std::string buffer;
  std::unordered_map<std::string, Command> commands;
};

#endif
