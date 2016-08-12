The Makeseed is one of three makefiles:  IncludeOS/src (-> os.a), Makeseed (->
your service) and IncludeOS/vmbuild/. For the individual service Makefiles (see
any ./test/.../integration or the examples) we got away pretty ok by just
including a master makefile. But that should also probably be done between the
main ./src Makefile and the make seed one, so that improvements such as setting
compiler from env can be fixed by changing only one file.
