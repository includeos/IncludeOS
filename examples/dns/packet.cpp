
#include "packet.hpp"

//Packet::packet_status Packet::status() { return _status; }

Packet::~Packet(){
  //debug("DESTRUCT packet, buf@0x%lx \n",(uint32_t)_data);
}

Packet::Packet(uint8_t* buf, uint32_t len, packet_status s):
_data(buf),_len(len),_status(s),_next_hop4(){}

//IP4::addr Packet::next_hop(){
//return _next_hop4;
//}
//void Packet::next_hop(IP4::addr ip){
//_next_hop4 = ip;
//}
