# IncludeOS Design choices


## Device management

### Constraints:

    1. We want this notation: 
    `Dev::eth(0)->onData.ip(callback); //Ethernet frames`
    `Dev::eth(0)->on.IPData(callback)`

    



## Draft 1
Devices are C++ objects, with the following hierarchy:


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

 Dev::eth(0).name()
 Dev::disk(0).name()
 
 Dev::disk(0).open("./myFile.txt",[](File f){printf(f.name());

 Dev::eth(0).on(NicEvent::req, []{printf("A new TCP event")});
 => No compiler support

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