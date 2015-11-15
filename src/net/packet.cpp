#define DEBUG

#include <os>
#include <net/packet.hpp>

using namespace net;

Packet::packet_status Packet::status()
{ return _status; }

Packet::~Packet(){
  debug("<Packet> DESTRUCT packet, buf@0x%x \n",(uint32_t)_data);
  release(_data);
}


Packet::Packet(uint8_t* buf, uint32_t len, packet_status s) 
  : _data(buf), _len(len), _status(s), _next_hop4() 
{
  debug("<Packet> Constructed. Buf @ %p \n",buf);
}

Packet::Packet(packet_status s) 
  : _data(bufstore().get()), _len(bufstore().bufsize()), _status(s), _next_hop4() 
{
  debug("<Packet> Constructed, with buffer from bufstore @ %p \n", _data);
}
    

IP4::addr Packet::next_hop(){
  return _next_hop4;
}

void Packet::next_hop(IP4::addr ip){
  _next_hop4 = ip;
}


int Packet::set_len(uint32_t l){
  // Upstream packets have length set by the interface
  // if(_status == UPSTREAM ) return 0; ... well. DNS reuses packet.
  if(l > _bufsize)
    return 0;
  _len = l;
  return _len;
}

void net::default_release(buffer b){
  Packet::bufstore().release(b);  
};


