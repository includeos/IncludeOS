#! /bin/sh

BRIDGE=include0
NETMASK=255.255.0.0
GATEWAY=10.0.0.1

# For later use
NETWORK=10.0.0.0
DHCPRANGE=10.0.0.2,10.0.0.254

brctl addbr $BRIDGE
ifconfig $BRIDGE $GATEWAY netmask $NETMASK up 
ifconfig include0 hw ether c0:01:70:01:00:01

