### IncludeOS demo in Linux Userspace

```
sudo mknod /dev/net/tap c 10 200
lxp-run
```

This demo service should start an instance of IncludeOS that brings up a minimal web service on port 80 with static content.

The default static IP is 10.0.0.42.
