#ifndef KERNEL_BTERM_HPP
#define KERNEL_BTERM_HPP

#include <functional>
#include <serial>

// some bidirectional communication interface
struct IBidirectional
{
  using on_read_func  = std::function<void(char)>;
  using on_write_func = std::function<void()>;
  
  virtual void set_on_read(on_read_func) = 0;
  virtual void write(char, on_write_func) = 0;
  
  on_read_func read_callback;
};

// communication using serial port
struct SerialComm : public IBidirectional
{
  SerialComm(hw::Serial& s);
  
  virtual void set_on_read(on_read_func) override;
  virtual void write(char) override;
  
  hw::Serial& serial;
};

class Terminal
{
public:
  Terminal(IBidirectional& iface)
    : comm(iface) {}
  
  
  
  IBidirectional& comm;
};

#endif
