#!/bin/sh

if [ $# -eq 0 ]
then
  echo ">>> Default settings "
  BRIDGE=include0
  NETMASK=255.255.0.0
  GATEWAY=10.0.0.1

elif [ $# -eq 3 ]
then
  BRIDGE=$1
  NEMASK=$2
  GATEWAY=$3
else
  me=`basename "$0"`
  echo "Usage: $me [name netmask gateway]"
  exit 1
fi

echo ">>> Creating bridge $BRIDGE, netmask $NETMASK, gateway $GATEWAY "

# Håreks cool hack:
# - First two bytes is fixed to "c001" because it's cool
# - Last four is the gateway IP, 10.0.0.1
HWADDR=c0:01:0a:00:00:01

# For later use
NETWORK=10.0.0.0
DHCPRANGE=10.0.0.2,10.0.0.254

# Check if bridge is created
if brctl show $BRIDGE 2>&1 | grep -q "No such device"; then
  echo ">>> Creating network bridge (requires sudo):"
  sudo brctl addbr $BRIDGE || exit 1
else
  echo ">>> Network bridge already created"
fi

# Check if bridge is configured
if ip -o link show $BRIDGE | grep -q "$HWADDR"; then
  echo ">>> Network bridge already configured"

  # Make sure that the bridge is activated
  if ip -o link show $BRIDGE | grep -q "UP"; then
    echo ">>> Network bridge already activated"
  else
    echo ">>> Activating network bridge (requires sudo):"
    sudo ifconfig $BRIDGE up || exit 1
  fi
else
  # Configure and activate bridge
  echo ">>> Configuring network bridge (requires sudo):"

  sudo ifconfig $BRIDGE $GATEWAY netmask $NETMASK up || exit 1
  sudo ifconfig $BRIDGE hw ether $HWADDR || exit 1
fi

exit 0
