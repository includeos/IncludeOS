#!/bin/sh

BRIDGE=bridge43
NETMASK=255.255.0.0
GATEWAY=10.0.0.1

# Håreks cool hack:
# - First two bytes is fixed to "c001" because it's cool
# - Last four is the gateway IP, 10.0.0.1
HWADDR=c0:01:0a:00:00:01

# For later use
NETWORK=10.0.0.0
DHCPRANGE=10.0.0.2,10.0.0.254

# Check if bridge is created
if uname -s | grep Darwin > /dev/null 2>&1; then  # Check if on Mac
  if ifconfig $BRIDGE 2>&1 | grep -q "does not exist"; then
    echo ">>> Creating network bridge (requires sudo):"
    sudo ifconfig $BRIDGE create || exit 1
  else
    echo ">>> Network bridge already created"
  fi
else
  if brctl show $BRIDGE 2>&1 | grep -q "No such device"; then
    echo ">>> Creating network bridge (requires sudo):"
    sudo brctl addbr $BRIDGE || exit 1
  else
    echo ">>> Network bridge already created"
  fi
fi

# Check if bridge is configured
#if ip -o link show $BRIDGE | grep -q "$HWADDR"; then
if ifconfig $BRIDGE | grep -q "$HWADDR"; then
  echo ">>> Network bridge already configured"

  # Make sure that the bridge is activated
  #if ip -o link show $BRIDGE | grep -q "UP"; then
  if ifconfig $BRIDGE | grep -q "UP"; then
    echo ">>> Network bridge already activated"
  else
    echo ">>> Activating network bridge (requires sudo):"
    sudo ifconfig $BRIDGE up || exit 1
  fi
else
  # Configure and activate bridge
  echo ">>> Configuring network bridge (requires sudo):"

  sudo ifconfig $BRIDGE $GATEWAY netmask $NETMASK up || exit 1
  if uname -s | grep Darwin > /dev/null 2>&1; then
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
