
#pragma once
#ifndef HTTP_ERROR_HPP
#define HTTP_ERROR_HPP

#include <string>

namespace http {

  class Error {
  public:
    enum Code {
      NONE,
      RESOLVE_HOST,
      NO_REPLY,
      INVALID,
      TIMEOUT,
      CLOSING
    };

    Error(Code code = NONE)
      : code_{code}
    {}

    operator bool() const
    { return code_ != NONE; }

    Error code() const
    { return code_; }

    bool timeout() const
    { return code_ == TIMEOUT; }

    std::string to_string() const
    {
      switch(code_)
      {
        case NONE:
          return "No error";
        case RESOLVE_HOST:
          return "Unable to resolve host";
        case NO_REPLY:
          return "No reply";
        case TIMEOUT:
          return "Request timed out";
        case CLOSING:
          return "Connection closing";
        default:
          return "General error";
      } // < switch code_
    }

  private:
    Code code_;

  }; // < class Error

} // < namespace http

#endif // < HTTP_ERROR_HPP
