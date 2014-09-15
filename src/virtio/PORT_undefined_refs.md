# PORTING virtio driver (`virtio.c` and `virtionet.c`) from SanOS to IncludeOS:

1. `[x]` Include all necessary headers
   * But since we don't need a fraction of what's in there, I created "hw/sanos_port.h", where I put a lot of stuff from various SanOS headers
   * I think all the #defines and structs in sanos_port.h should be portable now - there could be memory references in there which won't work, but as far as I've seen all memory allocation is done via kmalloc (now malloc)

2. `[x]` Make it valid C++ (mostly do explicit casts/coersions)

3. `[x]` Once all the definitions exists, `$(CPP) -c` works (virtio.o and virtionet.o were added to Makfile objectlist), and we're left with the missing references from the linker.
(output from  `$ make |& grep "undefined reference" | cut -d: -f3 | sort | uniq | cut -d' ' \f5-` below)

4. `[ ]` Explain what all of these do:

* `dev_make(char*, driver*, unit*, void*)`
* `dev_receive(short, pbuf*)`
* `device(short)`
* `get_unit_iobase(unit*)`
* `get_unit_irq(unit*)`
* `init_event(event*, int, int)`
* `pbuf_alloc(int, int, int)`
* `pbuf_chain(pbuf*, pbuf*)`
* `pbuf_dechain(pbuf*)`
* `pbuf_free(pbuf*)`
* `pbuf_header(pbuf*, int)`
* `pbuf_realloc(pbuf*, int)`
* `queue_irq_dpc(dpc*, void (*)(void*), void*)`
* `register_interrupt(interrupt*, int, int (*)(context*, void*), void*)`
* `set_event(event*)`
* `virt2phys(void*)`
* `wait_for_object(void*, unsigned int)`
