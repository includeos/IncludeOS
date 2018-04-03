### IncludeOS HTTP Client example

To start, using boot:
```
boot .
```

This example will send a GET request to http://www.google.com with our Basic_client, and to https://www.google.com with our HTTPS supported Client.
For the example to succeed, make sure your virtual machine can reach the internet (ip forward & nat).

For example purpose only. Don't reuse the `server.pem` certificate.
