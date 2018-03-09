set architecture i386:x86-64

break Service::start
break main

set non-stop off
target remote localhost:1234
set $eax=1
continue
