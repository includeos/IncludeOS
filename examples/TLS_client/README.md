### TLS client demo service

To start, using boot:
```
boot .
```

This demo-service should should connect with TLS 1.2 to 10.0.0.1:2345.

To test (from service folder):
```
openssl s_server -port 2345
```
