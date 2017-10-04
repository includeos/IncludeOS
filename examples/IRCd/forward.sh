#!/bin/bash

# Script for forwarding packets from IncludeOS running on a nested qemu interface
# This works on ubuntu 16.04
# Make sure ip forwarding is enabled by running: sysctl -w net.ipv4.ip_forward=1

OUTWARD_FACING_INTERFACE=enp2s0
BRIDGE_INTERFACE=bridge43

# Clear iptables
sudo iptables -t nat -F

# Masks the source address
sudo iptables -t nat -A POSTROUTING -o $OUTWARD_FACING_INTERFACE -j MASQUERADE

# Sets the packets coming from bridge43 to be natted
sudo iptables -t nat -A PREROUTING -i $BRIDGE_INTERFACE -p udp -m udp
