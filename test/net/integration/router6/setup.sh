#! /bin/bash
source_net=fe80:0:0:0:e823:fcff:fef4:0/112
source_bridge=bridge43

dest_net=fe80:0:0:0:abcd:abcd:1234:0/112
dest_bridge=bridge44
dest_gateway=fe80:0:0:0:abcd:abcd:1234:8367


export NSNAME="server1"
shopt -s expand_aliases
alias server1="sudo ip netns exec $NSNAME"

setup() {

  # TODO: it's probably not nice to install test deps here
  sudo apt-get -qqq install -y iperf3

  # Make sure the default bridge exists
  $INCLUDEOS_PREFIX/scripts/create_bridge.sh

  # Create veth link
  sudo ip link add veth_src type veth peer name veth_dest

  # Bring up source end
  sudo ip link set veth_src up

  # Add network namespace
  sudo ip netns add $NSNAME

  # Add destination to namespace
  sudo ip link set veth_dest netns $NSNAME

  # Bring up destination end, with IP, inside namespace
  server1 ip -6 addr add $dest_gateway dev veth_dest
  server1 ip link set veth_dest up
  server1 ip link set lo up

  # Create a second bridge and bring it up, no IP
  sudo brctl addbr $dest_bridge
  sudo ip link set dev $dest_bridge up

  # Add source end to bridge44
  sudo brctl addif $dest_bridge veth_src

  # Route all traffic to the isolated network via bridge43
  sudo ip -6 route add $dest_net dev $source_bridge

  # Route all traffic from server1 back to root namespace, via veth_dest
  server1 ip -6 route add $source_net dev veth_dest

}

undo(){
  echo ">>> Deleting veth devices"
  ip link delete veth_src
  ip link delete veth_src
  echo ">>> Deleting $dest_bridge"
  sudo ip link set $dest_bridge down
  sudo brctl delbr $dest_bridge
  echo ">>> Deleting namespace and veth pair"
  sudo ip netns del $NSNAME
  echo ">>> Deleting route to namespace"
  sudo ip -6 route del $dest_net dev $source_bridge
}


if [ "$1" == "--clean" ]
then
  undo
else
  setup
fi
