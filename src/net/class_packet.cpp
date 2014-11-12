//#define DEBUG

#include <os>
#include <net/class_packet.hpp>

using namespace net;

Packet::packet_status Packet::status()
{ return _status; }

Packet::~Packet(){
  debug("DESTRUCT packet, buf@0x%lx \n",(uint32_t)_data);
}

Packet::Packet(uint8_t* buf, uint32_t len):
  _data(buf),_len(len){}
