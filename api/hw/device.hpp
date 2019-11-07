#pragma once
#include <string>

namespace hw {

  class Device {
  public:
    enum class Type
    {
      Block,
      Nic
    };

    virtual void deactivate() = 0;
    virtual void flush() {} // optional

    virtual Type device_type() const noexcept = 0;
    virtual std::string device_name() const = 0;

    virtual std::string to_string() const
    { return to_string(device_type()) +  " " + device_name(); }

    static std::string to_string(const Type type)
    {
      switch(type)
      {
        case Type::Block: return "Block device";
        case Type::Nic:   return "NIC";
        default:          return "Unknown device";
      }
    }
  };

}
