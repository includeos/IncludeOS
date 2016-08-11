# TCP Test

### Usage:
1. Start the TCP test service, with `./test.sh`
2. Start the python test with `$ python test.py $GUEST_IP $HOST_IP `

Guest and Host IP are optional (default are GUEST=`10.0.0.42` HOST=`10.0.0.1`).

Script helps verify the TCP connections when the service is running.

To make outgoing internet connection work, correct forwarding rules needs to be setup.

#### Virtualbox
This test is developed on OS X using Virtualbox. Here's an recepie how to try it out.

1. make *(remember to export LD_INC and AR_INC)*
2. `/<includeos_repo>/etc/vboxrun.sh IncludeOS_TCP_Test.img TCP_test`
3. Kill VM.
4. Open Virtualbox => TCP_test => Settings => Network
5. Change *Adapter 1* to use *NAT* => Advanced => Port Forwarding
6. Add the following rules:

```
$ vboxmanage showvminfo TCP_test
...
NIC 1 Rule(0):   name = Rule 1, protocol = tcp, host ip = 127.0.0.1, host port = 8081, guest ip = , guest port = 8081
NIC 1 Rule(1):   name = Rule 2, protocol = tcp, host ip = 127.0.0.1, host port = 8082, guest ip = , guest port = 8082
NIC 1 Rule(2):   name = Rule 3, protocol = tcp, host ip = 127.0.0.1, host port = 8083, guest ip = , guest port = 8083
NIC 1 Rule(3):   name = Rule 4, protocol = tcp, host ip = 127.0.0.1, host port = 8084, guest ip = , guest port = 8084
```

Now run the VM again (step 2).

To verify it: `$ python test.py 127.0.0.1 127.0.0.1`
