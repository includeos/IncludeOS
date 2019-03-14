#! /bin/bash
set -e
source_net=fe80:0:0:0:e823:fcff:fef4:0/112
source_bridge=bridge43

dest_net=fe80:0:0:0:abcd:abcd:1234:0/112
dest_bridge=bridge44
dest_gateway=fe80:0:0:0:abcd:abcd:1234:8367

if1=tap0
if2=tap1

export NSNAME="server1"
shopt -s expand_aliases
alias server1="sudo ip netns exec $NSNAME"

setup() {
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
  sudo ip link add name $dest_bridge type bridge
  sudo ip link set dev $dest_bridge up

  # Add source end to bridge44
  sudo ip link set dev veth_src master $dest_bridge

  # Route all traffic to the isolated network via bridge43
  sudo ip -6 route add $dest_net dev $source_bridge

  # Route all traffic from server1 back to root namespace, via veth_dest
  server1 ip -6 route add $source_net dev veth_dest
  echo ">>> Setup complete"
}

undo(){
  # Always run all cleanup commands even if one fails
  set +e
  echo ">>> Deleting veth_src"
  sudo ip link delete veth_src
  echo ">>> Deleting $dest_bridge"
  sudo ip link set $dest_bridge down
  sudo ip link del $dest_bridge
  echo ">>> Deleting namespace and veth pair"
  sudo ip netns del $NSNAME
  echo ">>> Deleting route to namespace"
  sudo ip -6 route del $dest_net dev $source_bridge
}

vmsetup(){
  echo ">>> Moving VM iface $if2 to $dest_bridge"
  sudo ip link set dev $if2 nomaster
  sudo ip link set dev $if2 master $dest_bridge
  sudo ip link set $if2 up
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
