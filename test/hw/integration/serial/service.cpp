
#include <os>
#include <hw/serial.hpp>

void Service::start()
{
  auto& com1 = hw::Serial::port<1>();

  com1.on_readline([](const std::string& s){
      CHECK(true, "Received: %s", s.c_str());

    });
  INFO("Serial Test","Doing some serious serial");
  printf("trigger_test_serial_port\n");
}
