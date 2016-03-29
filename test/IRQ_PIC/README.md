# Test basic PIC / IRQ functionality

Test of the following:

1. Soft-triggering IRQ's will call interrupt-handlers, and in turn delegates
2. Serial port, IRQ 4: the service will read from serial port. With Qemu `-nographic` this will automatically be connected to stdin. Any character written to the serial port should result in a checkbox'ed receipt e.g. `[x] Serial port (IRQ 4) received 'a'`
3. UDP, IRQ 11: There's a UDP server set up, making it possible to test IRQ 11. Any data written to IP 10.0.0.42, UDP port 4242, should result in a checkbox'ed receipt, e.g.
```
$ echo "asdf" | nc -u 10.0.0.42 4242
-> [x] UDP received: 'asdf'
```
4. A 1-second interval timer is continously running (IRQ 0). Each second it should output a chekced receipt with an incremented counter.

For this setup it's expected that no combination of IRQ's should prevent other IRQ's from triggering. 

No automation yet.
