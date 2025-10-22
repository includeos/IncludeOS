# Test Gateway

## Nics
```
0: virtio (eth0)
1: virtio (eth1)
2: vmxnet3 (host1)
3: vmxnet3 (host2)
```

## Routing with NAT firewall

Network setup:
```
Nics:
0: 10.0.1.1/24 (no gateway)
1: 10.0.2.1/24 (no gateway)
2: 10.0.1.10/24 gateway: 10.0.1.1
3: 10.0.2.10/24 gateway: 10.0.2.1

Routes:
10.0.1.0/24 eth0
10.0.2.0/24 eth1

Diagram:
host1 <-> [eth0 <-> eth1] <-> host2
```

Will test routing mostly between `host1` and `host2` that's only reachable through our "router" (`eth0 <-> eth1`) e.g.:
* Ping host1 => host2
* Ping host1 => eth0
* Ping host1 => eth1
* Ping host2 => host1

Will also test DNAT/SNAT (see nacl.txt).

## VLAN with routing

Mirror the setup from above, but with 100 VLAN connected to 100 VLAN on the left side `host1 <-> eth0`.
Configure the network where the 3rd octet is the same as the VLAN tag: `vlan 0.x: 10.0.x.1`.

```
eth0(0):
// 10.0.10.1 - 10.0.109.1
0.10: 10.0.10.1/24 route: 10.0.10.0/24 eth0.10
...
0.109: 10.0.109.1/24 route: 10.0.109.0/24 eth0.109

host1(2):
// 10.0.10.10 - 10.0.109.10
2.10: 10.0.10.10/24 gateway: 10.0.10.1
...
2.109: 10.0.109.10/24 gateway: 10.0.109.1

eth1(1):
1.1337: 10.0.224.1/24 route: 10.0.224.0/24 eth1.1337

host2(3):
3.1337: 10.0.224.10/24 gateway: 10.0.224.1
```

Will test routing with VLAN by listening on tcp port 4242 on `host2.1337` and expect 100 new connections.
* TCP connect host1.10 => host2.1337 (10.0.10.10 => 10.0.224.10)
* etc...
