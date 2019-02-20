#! /bin/bash
set -e #abort on first command returning a failure

source_net=10.0.0.0/24
source_bridge=bridge43

dest_net=10.42.42.0/24
dest_bridge=bridge44
dest_gateway=10.42.42.2

if1=tap0
if2=tap1

export NSNAME="server1"
shopt -s expand_aliases
alias server1="sudo ip netns exec $NSNAME"

setup() {

  # TODO: it's probably not nice to install test deps here
  #sudo apt-get -qqq install -y iperf3

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
  server1 ip addr add $dest_gateway/24 dev veth_dest
  server1 ip link set veth_dest up
  server1 ip link set lo up

  # Create a second bridge and bring it up, no IP
  sudo brctl addbr $dest_bridge
  sudo ip link set dev $dest_bridge up

  # Add source end to bridge44
  sudo brctl addif $dest_bridge veth_src

  # Route all traffic to the isolated network via bridge43
  sudo ip route add $dest_net dev $source_bridge

  # Route all traffic from server1 back to root namespace, via veth_dest
  server1 sudo ip route add $source_net via $dest_gateway

}


undo(){
  echo ">>> Deleting veth devices"
  sudo ip link delete veth_src
  sudo ip link delete veth_dest
  echo ">>> Deleting $dest_bridge"
  sudo ip link set $dest_bridge down
  sudo brctl delbr $dest_bridge
  echo ">>> Deleting namespace and veth pair"
  sudo ip netns del $NSNAME
  echo ">>> Deleting route to namespace"
  sudo ip route del $dest_net dev $source_bridge

}

vmsetup(){
  echo ">>> Moving VM iface $if2 to $dest_bridge"
  sudo brctl delif $source_bridge $if2
  sudo brctl addif $dest_bridge $if2
  sudo ifconfig $if2 up
  echo ">>> Done."

}

if [ "$1" == "--clean" ]
then
  undo
elif [ "$1" == "--vmsetup" ]
then
  vmsetup
else
  setup
fi
