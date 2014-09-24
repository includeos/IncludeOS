# IncludeOS Design choices


## Device management


### Constraints:

1. **Notation:** We want notation like this:
   * A) `Dev::eth(0).on(IP::data, callback)`
   
   * Or something like
     * B) `Dev::eth(0).port(80).on(
     * C) `Dev::eth(0).onData.ip(callback); //Ethernet frames`
     * D) `Dev::eth(0).on.IPData(callback)`


2. **Private constructors**: We don't want the Nic constructor to be available for the user (but it has to be available in the ABI). 
    
3. **No-cost Polymorphism**: We want a Nic to have several events, implemented accross several drivers. (The normal way would be to just have a Nic superclass, and polymorphically make subclasses, but then there's a chance of polymorphism overhead. ... allthough we could make sure never to use- or return general "Nic" pointers). 

4. **No Overhead principle**: If I don't use a driver, I don't pay for one, i.e. it does not get included into the code.

5. (**Failsafe operation**): It should ... kind of ... be possible for one to ask for a device that doesn't exist. Should it though? If your program wants to write to a file but there's no disk - do you want to continue? Well, throwing an exception if they asked for the wrong Nic would be nice.

#### Example: 
  
  We want the IP stack to be able to subscribe to Nic-events as well. So, when we say:

   ```
   Dev::eth(0).port(80).on(HttpRequest,callback(...));
    
   ```
  
  We want:

  A) `Dev` to create a Nic - if it doesn't exist (1 and 2 OK)
  B) The event subscription to propagate all the way down to the Nic DRIVER like so:
  
    ```
     HttpHandler: port(80).on(TCP::Data,this->notify);
       |
       +-> TCPHandler: port(80).on(IP::Data, this->notify);
             |
             +-> IPHandler: DRIVER.on(Eth::Data, this->notify); //Driver == EthHandler
                   |
                   +-> DRIVER: EnableIRQ if not done & register 
                         |
                         +-> PIC::IRQ(16, delegate(this.data));
    ```
    
    The callback propagation will then look like this:

    ```
    Calls: 

            IRQ
             |
     1       +-> DRIVER::data(buf*)
                   |
     2             +-> IPHandler.data(frames*) // i.e. foreach(callback) callback(); 
                         : (if valid)
                         |
     3                   +-> TCPHandler.data(packet*) // i.e. foreach(callback) callback();
                               : (if valid)
                               |
     4                         +-> HttpHandler.data(contents*)
                                     : (if valid)
                                     |
     5                               +-> onRequest(HttpRequest req, HttpResp resp);

    ```

### Conclusions:

What's the minimum number of function calls? Well, 5, but we're not *always* going to notify the layer above, right? So there's either a flag check, or a pointer check, and then a function call. 

* Is `if(callback) callback(); ` slower than `if(notify_layer_n) notify_n()` ? Yes - one cycle or something, since it's indirection (callback is a pointer).

* Is `foreach(callbacks call) call()` slower than `if(notify_layer_n) notify_n(); if(notify_layer_m) notify_m()`? Probably, again there's indirection.

So: Is indirection a problem? And, is it at all avoidable? It's avoidable if we don't mind never replacing or decoupling the layer instances. Problem? Well, if it is, we can have it both ways; a tight coupling for layer2layer, and then an extra event-queue for each layers events, for external subscriptions. We can start with the loose coupling, and tighten later.


For each call through the stack, add 5 memory lookups if there's indirection at every step. 5 `mov`s...

```
 +-------------+  
 | HTTPHandler |
 +-------------+     +-------------+                        Virtio  Intel ...
 |             +<--->| TCPHandler  |                            \   /
 +-------------+     +-------------+     +-------------+         \ /
                     |             +<--->+ IPHandler   |          V
                     +-------------+     +-------------+          |
                                         |             +<--->[ DRIVER ] 
                                         +-------------+              
```


So, do we have to do like [STM32Plusnet](https://github.com/andysworkshop/stm32plus)?

i.e. `HTTPHandler<TCPHandler<IPHandler<Virtio>>>` ?

Other options, disadvantages / advantages?

* We're getting compile-time polymorphism
  * But we're also getting overhead (including more drivers than necessary), if we're doing runtime detection of NIC's. ... But there's no way to get runtime detection without including all (both) the drivers.

* How do we allow a user to subscribe to events from somewhere down in the stack? Probably by having the Nic have independent pointers to each layer. 

* So far it seems fine to have a `Nic<DRIVER>` template class, and then build a stack like `layerA< layerB<DRIVER> ... >`. But, we also want private constructor; just to make sure there's some governance. If you instantiate `Nic<DRIVER> nic1` and `Nic<DRIVER> nic2`, they would either register on the same PCI-Nic, or they would have to register at a common class (like Dev) anyway, in which case Nic would have to be a friend of Dev, or Dev would have to have a public registry-function.


* Drawbacks: 
  * Much of the stack has to be in header files. (But, we can write all the specializations ourselves if we want)
  * We can't overload names in `Dev` (like Dev::eth) without templatizing either the `Dev` class or the name (like `Dev<VirtioNet>::eth()` or `Dev::eth<VirtioNet>(n)`

* Where to templatize: **A)** `Dev::eth<VIRTIO>(0)` or **B)** `Nic<Virtio> Dev::eth(0)` ? 
    * A) Con: Is ugiler - you always have to specify the type. 
    * B) What happens with just `Dev::eth(0).name?`



## Draft 1
Devices are C++ objects, with the following hierarchy:
(Forget looking at this in Doxygen)

**Notation:**
* `[ABI]` - accessible via the ABI
* `(Private to OS)` - invisible.

```    
  [Dev]
    |
    +---------(PCI_Device)
    :             |   
    ::eth(n)......+-- [Nic] 
    :             |     |
    :             |     +-- (Virtio_Nic)
    :             |     |
    :             |     +-- (Some physical_Nic)
    :             |
    ::disk(n).....+-- [Disk]
    :             |
    ::serial(n)...+-- [Serial]
                  |
                  +-- Other (Buses, Display etc.)
```

Template solution:

`Dev::eth(0).name()`
`Dev::disk(0).name()`
`Dev::disk(0).open("./myFile.txt",[](File f){printf(f.name());`
`Dev::eth(0).on(NicEvent::req, []{ printf("A new TCP event") });`
 => No compiler support. Well. 

 Dev::eth(0).onTCPReq([]{printf("A new TCP event")});
 => Compiler support - but requires Dev::eth(0) to implement onTCPReq(callback);
    


### Device access from the public ABI
Having an intuitive ABI by whch to access devices is priority nr.1, *as long as efficiency does not suffer notably*. 



### Inheritance or members?
A nice way to do this would be by way of polymorphism. However, that comes with a performance penalty. [TODO: Measure.]

### Dynamic or static allocation?
* Static allocation is faster
  * But then we need to know the number of devices of each type in advance. It's possible to have the user decide; for services that won't be hot-plugging devices we could set the number of expected devices of each type at compile-time.
* Dynamic allocation
  * This means we have to have the Device class keep an array of pointers to each type, and not an array of actual devices.

### Pointer or reference?
If a user tries to access a device that does not exist (i.e. it was not found when the PCI manager probed for it) we need to:
* A) Throw an exception
     * Which is our only choice if we return a reference.
     * Then we need to throw exceptions inside `Device::get()`
* B) Return a null pointer
     * Which we only can if we return a pointer.
* C) Just panic. That means the user can't write code to "probe around" for existing nics. Problem? Don't know. Yea, probably. Would be more robust to throw exceptions.


### Conclusions:
* A) We want a Nic to be able to register itself in the Device class, via a private member add_Nic(). Why can't the Device class do this? Then the Device class would have to do the probing. In order to keep the Device class clean an readable in the ABI, We don't want that,  - we want the "hidden" PCI manager to do it. Hence, **a Nic must inherit "Device"** - protected then... or via a public constructor. 
  `: Device(Nic* Nic) ...`
  `: Device(Disk* Disk) ...`

* 