set architecture i386:x86-64

break OS::start
break Service::start
break main

set step-mode on
set non-stop off
target remote localhost:1234

# To use the GDB_ENTRY macro, uncomment:
set $eax=1
