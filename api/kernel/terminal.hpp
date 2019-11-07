
#pragma once
#ifndef API_KERNEL_TERMINAL_HPP
#define API_KERNEL_TERMINAL_HPP

#include <terminal>
#include <net/inet>
#include <cstdint>
#include <cstdarg>
#include <unordered_map>
#include <vector>

class Terminal {
public:
  using Connection_ptr = net::tcp::Connection_ptr;
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

  Terminal(Connection_ptr);

  __attribute__((format (printf, 2, 3)))
  void write(const char* str, ...)
  {
    va_list args;
    va_start(args, str);
    char buffer[1024];
    int bytes = vsnprintf(buffer, 1024, str, args);
    va_end(args);

    stream->write(buffer, bytes);
  }
  void prompt();
  int  exec(const std::string& cmd);
  void close();

  static void register_program(std::string name, TerminalProgram);

  auto get_stream() {
    return stream;
  }

private:
  void command(uint8_t cmd);
  void option(uint8_t option, uint8_t cmd);
  void read(const char* buf, size_t len);
  void register_basic_commands();
  void intro();

  Connection_ptr stream;
  bool    iac     = false;
  bool    newline = false;
  uint8_t subcmd  = 0;
  std::string buffer;
};

#endif
