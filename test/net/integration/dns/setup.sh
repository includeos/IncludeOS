#!/bin/bash

# Sets up the environment needed for running the DNS test
OUTWARD_FACING_INTERFACE=ens3
BRIDGE_INTERFACE=bridge43

# Enable ip forwarding
sudo sysctl -w net.ipv4.ip_forward=1

# Create iptables rules for NATing all DNS requests

# Masks the source address
sudo iptables -t nat -A POSTROUTING -o $OUTWARD_FACING_INTERFACE -j MASQUERADE

# Udp packets coming from bridge43 should be natted
sudo iptables -t nat -A PREROUTING -i $BRIDGE_INTERFACE -p udp -m udp
