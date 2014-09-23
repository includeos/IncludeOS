# PORTING virtio driver from SanOS to IncludeOS
(`virtio.c` and `virtionet.c`)

1. `[x]` Include all necessary headers
   * But since we don't need a fraction of what's in there, I created "hw/sanos_port.h", where I put a lot of stuff from various SanOS headers
   * I think all the #defines and structs in sanos_port.h should be portable now - there could be memory references in there which won't work, but as far as I've seen all memory allocation is done via kmalloc (now malloc)

2. `[x]` Make it valid C++ (mostly do explicit casts/coersions)

3. `[x]` Once all the definitions exists, `$(CPP) -c` works (virtio.o and virtionet.o were added to Makfile objectlist), and we're left with the missing references from the linker.
(output from  `$ make |& grep "undefined reference" | cut -d: -f3 | sort | uniq | cut -d' ' \f5-` below)

4. `[x]` Explain what all of these do - ~~strikethrough~~ means we don't need it:

* `dev_make(char*, driver*, unit*, void*)`
  * "Constructor" for the 'dev' struct, attaching a 'unit' to a 'driver'. We'll make an OOP-version
* `dev_receive(short, pbuf*)`
  * A wrapper for the "receive" member of the "device class"; recieve data from deviceno to buf. 
* `device(short)`
  * A "getter" getting a dev* from the global device table.
* `get_unit_iobase(unit*)`
  * Getting the IO-base adress for a unit. Place OOP-version inside PCI_Unit class.
* `get_unit_irq(unit*)`
  * As above - get the IRQ of a given (PCI) unit. Make OOP-version inside PCI Unit class.
* `init_event(event*, int, int)`
  * Initialize an "event object" in Ringaards "Object Oriented C" terminology. It just initializes the object struct to an "event". An event seems to be exactly a *blocking lock*. Or... at least a "subclass" of an "object", which has a waitlist.
  * Defined in: `object.h`/`object.c`
  * Used by: `virtio_queue_init()` in `virtio.cpp` to signal that the buffer is available. 
* All the ones below are from `pbuf.h`. It's a packet buffering system, for layers *transport*, *IP*, *Link* and *RAW*.
  * `pbuf_alloc(int, int, int)`
  * `pbuf_chain(pbuf*, pbuf*)`
  * `pbuf_dechain(pbuf*)`
  * `pbuf_free(pbuf*)`
  * `pbuf_header(pbuf*, int)`
  * `pbuf_realloc(pbuf*, int)`
* `queue_irq_dpc(dpc*, void (*)(void*), void*)`
  * Defined in: `sched.c`. (*DPC => "Deferred Procedure Call"*)
  * Used by the virtio IRQ handler (`virtio_handler()` in virtio.cpp) to push a task (a function) onto a que, to be handled later (i.e. not by the interrupt handler itself). This is a GOOD way to do it, exactly what I was thinking for us. We want to return from the irq-handler asap. in order to allow more, so we just queue up the work and "schedule it" for later. In this case it queues a call to `virtio_dpc`, also in `virtio.cpp`. That call will notify all `virtio_queue`s on the `virtio_device` in question. 
* `register_interrupt(interrupt*, int, int (*)(context*, void*), void*)`
  * Equivalent to our `create_gate` in the IRQ handler class. Insert a gate into IDT. 
  * Defined in: `sanos_port.h` (see above - we have an equivalent)
  * Used by: `virtio_device_init()` in `virtio.cpp` to register a virtio IRQ. More generally `virtio_device_init()` talks directly to the PCI-unit supplied (detected previously by pci probing), to get the IRQ number (again supplied by Qemu), the iobase address etc.
* `set_event(event*)`
  * Set the event object to "signalled" - that means release all threads waiting for this event. 
* `wait_for_object(void*, unsigned int)` *The void ptr. is an object_t*
  * Wait for the object, to be "signalled" - i.e. wait for an event. So, this is like "lock.wait".
  * Defined in: `object.h`/`object.c`
  * Used by: `virtio_enqueue()` in `virtio.cpp` to wait for free buffers in the virtio queue. When signalled, add data to the queue. If there's still free space after adding, signal again by calling `set_event(&vq->bufavail)`.
* ~~`virt2phys(void*)`~~
  * Convert virtual address to physical. Nothing we need.

## What we'll need to do in IncludeOS

1. Make an equivalent of the whole DPC-system
  - This can be much simpler for us, since we're not multithreaded. The fact that Sanos uses DPC actually makes everything easier for us; this means that the only place we need to deal with concurrency is in our interrupt handler, exactly when we queue a task. When we're back in the wait-loop, we can single-threadedly call all the dpc-functions, which then won't be concurrent. 

2. Consider OOP'ing the whole unit/device system. Its a bit of work, but it will give us a much better understanding of what's going on, plus allow us to remove everything we don't need.


## Getting on with it

* Made a rudimentary OOP-structure
* I'm getting some virtio-specific data as expected from the PCI device

### How to get the mac address
In SanOS, it's quite horribly like this:

 1. `virtio_install` in `virtionet.c` calls:
    `virtio_get_config(struct virtio_device *vd, void *buf, int len)` like so:
    `virtio_get_config(&vnet->vd, &vnet->config, sizeof(vnet->config));`

    So, the "&bnet->config" struct gets filled into a buffer `buf`
     
 2. `virtio_install` calls `dev_make` from `dev.h`:
     `dev_make(char *name, struct driver *driver, struct unit *unit, void *privdata)`
     like so:
     `vnet->devno = dev_make("eth#", &virtionet_driver, unit, vnet);`
     
     In other words, the `vnet->config` struct ends up "serialized" in `dev->privdata`.
     
     That's where the mac-address is.

####Plan: Make a new version of `virtio_get_config`. 
OK, that worked really well. Turns out Ringaard has this virtio-stuff down much better than OSdev. (The mac-address is located at iobase + 0x20, together with the status)
