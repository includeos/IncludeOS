#include <hw/serial.hpp>

#undef DEBUG

using namespace hw;

// Storage for port numbers
constexpr uint16_t Serial::ports_[];


void Serial::init(){
  hw::outb(port_ + 1, 0x00);    // Disable all interrupts
  hw::outb(port_ + 3, 0x80);    // Enable DLAB (set baud rate divisor)
  hw::outb(port_ + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
  hw::outb(port_ + 1, 0x00);    //                  (hi byte)
  hw::outb(port_ + 3, 0x03);    // 8 bits, no parity, one stop bit
  hw::outb(port_ + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
  hw::outb(port_ + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

Serial::Serial(int port) : 
  nr_{port}, 
  port_{ port < 5 ? ports_[port -1] : 0 }
    {
      //init();
    }

void Serial::on_data(on_data_handler del){
  enable_interrupt();    
  on_data_=del;
  IRQ_manager::subscribe(irq_, irq_delg::from<Serial,&Serial::irq_handler_>(this) );
  INFO("Serial", "Subscribing to IRQ %i \n",irq_);
}

void Serial::on_readline(on_string_handler del, char delim){
  newline = delim;
  on_readline_ = del;
  on_data(on_data_handler::from<Serial,&Serial::readline_handler_>(this));
  INFO("Serial::on_readline", "Subscribing to data %i \n",irq_);
}


void Serial::enable_interrupt(){
  outb(port_ + 1, 0x01);
  IRQ_manager::eoi(irq_);  
}

void Serial::disable_interrupt(){
  outb(port_ + 1, 0x00);
}
    
char Serial::read(){
  return hw::inb(port_);
}

void Serial::write(char c){
  while (is_transmit_empty() == 0);
  hw::outb(port_, c);
}
    
int Serial::received(){
  return hw::inb(port_ + 5) & 1;
}

int Serial::is_transmit_empty() {
  return hw::inb(port_ + 5) & 0x20;
}


void Serial::irq_handler_ () { 
  IRQ_manager::eoi(irq_);
  
  while (received())
    on_data_(read());
    
}


void Serial::readline_handler_ (char c) { 
  
  if (c != newline) {
    buf += c;
    write(c);
    return;
  }
  
#ifdef DEBUG
  for ( auto& ch : buf )
    debug(" 0x%x | ", ch);
#endif 
  
  // Call the event handler
  on_readline_(buf);    
  buf.clear();
  
};
