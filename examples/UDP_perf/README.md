# IncludeOS UDP Performance
Runs a udp server or a client as a IncludeOS unikernel

#Pre-requistes:
Requires ncat

## Howto run as a Server:
On a terminal run: boot --create-bridge .
On other terminal run: ./send.sh

## Howto run as a Client:
On a terminal run: ./receive.sh
On other terminal run: boot --create-bridge . client

## How sampling is done
Sampling is collected approximately every 5 seconds when the unikernel is run as a server
Sampling is collected approximately every second when the unikernel is run a client. The test runs for 10 seconds.
