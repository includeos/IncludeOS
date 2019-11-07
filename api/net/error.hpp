
#ifndef NET_ERROR_HPP
#define NET_ERROR_HPP

#include <cstdint>
#include <string>

namespace net {

  /**
   *  General Error class for the OS
   *  ICMP_error f.ex. inherits from this class
   *
   *  Default: No error occurred
   */
  class Error {
  public:

    enum class Type : uint8_t {
      no_error,
      general_IO,
      ifdown,
      ICMP,
      timeout
      // Add more as needed
    };

    Error() = default;

    Error(Type t, const char* msg)
      : t_{t}, msg_{msg}
    {}

    virtual ~Error() = default;

    Type type()
    { return t_; }

    operator bool() const noexcept
    { return t_ != Type::no_error; }

    bool is_icmp() const noexcept
    { return t_ == Type::ICMP; }

    virtual const char* what() const noexcept
    { return msg_; }

    virtual std::string to_string() const
    { return std::string{msg_}; }

  private:
    Type t_{Type::no_error};
    const char* msg_{"No error"};

  };  // < class Error

} //< namespace net

#endif  //< NET_ERROR_HPP
