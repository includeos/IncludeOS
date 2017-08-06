# Test net::BufferStore and net::Packet chaining

Internal tests that verifies that packet chaining plays well with the buffer store, i.e. that you can chain lots of packets, dechain, and they all return their buffers back to the bufstore.
