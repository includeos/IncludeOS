### microLB demo

Start the nodeJS demo services first:
```
node server.js
```

Build and run the load balancer:
```
boot . --create-bridge
```

Connect to the load balancer:
```
curl 10.0.0.42
```

The load balancer should be configured to round-robin on 10.0.0.1 ports 6001-6004.
