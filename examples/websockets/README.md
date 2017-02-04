### WebSocket Example Service

To start:
```
boot .
```

Install python3 wsstat and run:
```
wsstat ws://10.0.0.42:8000 -n 500
```

This example should start an instance of IncludeOS that brings up a minimal websocket server (port 8000) as well as a simple client (that connects outwards on port 8001).
