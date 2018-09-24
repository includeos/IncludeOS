# Test services
More or less in progress, some work some don't.

The idea is to have various services here, which test different functionality, and that can be automated.

## IP addresses in use by test services

The following IP addresses are used by the services in the test folder. Make sure to use an unused one to avoid collisions when running tests in parallel.

### Network tests
- bufstore: No IP
- configure: 10.0.0.60, 10.0.0.61, 10.0.0.62
- dhclient: Time_sensitive, leave for now
- dhcpd: 10.0.0.9, 10.0.0.10, 10.0.0.11, 10.0.0.12
- dhcpd_dhclient_linux: leave for now
- dns: 10.0.0.48
- gateway: 10.0.1.1, 10.0.1.10, 10.0.2.1, 10.0.2.10
- http: 10.0.0.46
- icmp: 10.0.0.45
- icmp6: 10.0.0.52
- microLB: 10.0.0.1, 10.0.0.68, 10.0.0.69 - Is 10.0.0.1 a problem?
- nat: 10.1.0.1, 192.1.0.1, 10.1.0.10, 192.1.0.192, 10.1.10.20
- router: Intrusive + time sensitive - runs alone
- tcp: 10.0.0.44
- transmit: 10.0.0.49
- udp: 10.0.0.55
- vlan: 10.0.0.50, 10.50.0.10, 10.60.0.10, 10.0.0.51, 10.50.0.20, 10.60.0.20
- websocket: 10.0.0.54

### Others
- stress: 10.0.0.42
- plugin/unik: 10.0.0.56
- kernel/modules: 10.0.0.53
- posix/syslog_plugin: 10.0.0.47, 10.0.0.2
- posix/tcp: 10.0.0.57, 10.0.0.4
- posix/udp: 10.0.0.58, 10.0.0.3
- kernel/liveupdate: 10.0.0.59
- kernel/term: 10.0.0.63
