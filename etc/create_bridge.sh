#! /bin/sh

BRIDGE=include0
NETMASK=255.255.0.0
GATEWAY=10.0.0.1

# For later use
NETWORK=10.0.0.0
DHCPRANGE=10.0.0.2,10.0.0.254

BRSHOW="brctl show"

# Check if BRSHOW is in user path
if ! command -v brctl > /dev/null 2>&1; then
    BRSHOW="sudo $BRSHOW"
fi

# Check if bridge already is created
if $BRSHOW $BRIDGE 2>&1 | grep --silent "No such device"; then
  sudo brctl addbr $BRIDGE || exit 1
fi

sudo ifconfig $BRIDGE $GATEWAY netmask $NETMASK up || exit 1

# Håreks cool hack:
# - First two bytes is fixed to "c001" because it's cool
# - Last four is the gateway IP, 10.0.0.1
sudo ifconfig include0 hw ether c0:01:0a:00:00:01 || exit 1

exit 0
