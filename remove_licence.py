#!/usr/bin/python
import sys
outfile = sys.argv[1]
print(outfile + " is being processed...")

list = ()
with open("remove.txt", "r") as f:
    list = f.readlines()


with open(outfile, "r+") as f:
    d = f.readlines()
    f.seek(0)
    for i in d:
        if i not in list:
            f.write(i)
    f.truncate()
