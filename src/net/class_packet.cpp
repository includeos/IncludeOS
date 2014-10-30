#include <os>
#include <net/class_packet.hpp>

using namespace net;

Packet::packet_status Packet::status()
{ return _status; }
