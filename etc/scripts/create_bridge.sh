#!/bin/sh
set -e

echo "    Creating network bridge for IncludeOS "

if [ $# -eq 0 ]
then
  echo "    Using default settings "
  BRIDGE=bridge43
  NETMASK=255.255.255.0
  GATEWAY=10.0.0.1
  NETMASK6=64
  GATEWAY6=fe80::e823:fcff:fef4:83e7
  # Håreks cool hack:
  # - First two bytes is fixed to "c001" because it's cool
  # - Last four is the gateway IP, 10.0.0.1
  HWADDR=c0:01:0a:00:00:01

elif [ $# -eq 4 ]
then
  BRIDGE=$1
  NETMASK=$2
  GATEWAY=$3
  HWADDR=$4
else
  me=`basename "$0"`
  echo "Usage: $me [name netmask gateway hwaddr]"
  exit 1
fi

if [ -n "$INCLUDEOS_BRIDGE" ]; then
     BRIDGE=$INCLUDEOS_BRIDGE
fi

echo "    Creating bridge $BRIDGE, netmask $NETMASK, gateway $GATEWAY "
if [ -n "$GATEWAY6" ]; then
echo "    ipv6 netmask $NETMASK6, gateway $GATEWAY6 "
fi

# For later use
NETWORK=10.0.0.0
DHCPRANGE=10.0.0.2,10.0.0.254

# Check if bridge is created
if uname -s | grep Darwin > /dev/null 2>&1; then  # Check if on Mac
  if ifconfig $BRIDGE 2>&1 | grep -q "does not exist"; then
    echo "    Creating network bridge (requires sudo):"
    sudo ifconfig $BRIDGE create || exit 1
  else
    echo "    Network bridge already created"
  fi
else
  BRSHOW="brctl show"

  # Check if BRSHOW is in user path
  if ! command -v brctl > /dev/null 2>&1; then
      BRSHOW="sudo $BRSHOW"
  fi

  # Check if bridge already is created
  if $BRSHOW $BRIDGE 2>&1 | grep -qE "No such device|bridge $BRIDGE does not exist"; then
    echo "    Creating network bridge (requires sudo):"
    sudo brctl addbr $BRIDGE || exit 1
  else
    echo "    Network bridge already created"
  fi
fi

IFCONFIG="ifconfig"
if ! command -v ifconfig > /dev/null 2>&1; then
  IFCONFIG="sudo $IFCONFIG"
fi

# Check if bridge is configured
#if ip -o link show $BRIDGE | grep -q "$HWADDR"; then
if $IFCONFIG $BRIDGE | grep -q "$HWADDR"; then
  echo "    Network bridge already configured"

  # Make sure that the bridge is activated
  #if ip -o link show $BRIDGE | grep -q "UP"; then
  if $IFCONFIG $BRIDGE | grep -q "UP"; then
    echo "    Network bridge already activated"
  else
    echo "    Activating network bridge (requires sudo):"
    sudo ifconfig $BRIDGE up || exit 1
  fi
else
  # Configure and activate bridge
  echo "    Configuring network bridge (requires sudo):"

  sudo ifconfig $BRIDGE $GATEWAY netmask $NETMASK up || exit 1
  if [ -n "$GATEWAY6" ]; then
    sudo ifconfig $BRIDGE inet6 add $GATEWAY6/$NETMASK6
  fi
  if uname -s | grep Darwin > /dev/null 2>&1; then
    echo "    Setting ether to $HWADDR"
	sudo ifconfig $BRIDGE ether $HWADDR || exit 1
  else
	sudo ifconfig $BRIDGE hw ether $HWADDR || exit 1
  fi
fi

# Check how many routes are available
routes=`netstat -rnv | grep -c 10.0.0.0`
if [ $routes -gt 1 ]; then
  echo Potential ERROR: More than 1 route to the 10.0.0.0 network detected
  echo Check the interfaces using ifconfig and turn off any potential
  echo conflicts. The bridge interface in use is: $BRIDGE
  echo to disable use the command: ifconfig "<iface>" down
fi

exit 0
