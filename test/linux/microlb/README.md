###

```
$ ./create_bridge.sh bridge44 255.255.255.0 10.0.1.1 c0:ff:0a:00:00:01
$ sudo ip addr flush dev bridge43
$ sudo ip addr flush dev bridge44
$ nodejs server.js
```
