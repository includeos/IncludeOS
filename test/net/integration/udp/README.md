# Test UDP functionality

This service sets up a UDP "parrot" i.e. a UDP server that listens to a certain port, and echos back to you anything you send to it.

### TODO

1. Make an external test script (or a test program using e.g. google test) that writes random byte sequences in, and verifies that the output is identical. 
2. Find out what happens when there is more data than one frame can hold (i.e. 1500 bytes minus headers) - and compare with what sould happen, i.e. what happens with a normal C-socket in Linux.




