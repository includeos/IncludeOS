#!/usr/bin/env python3

from __future__ import print_function
from builtins import str
import sys
import argparse
from subprocess import call
from sys import platform as _platform

parser = argparse.ArgumentParser(description='Create memdisk')
parser.add_argument('disk', metavar='DISK', default=None, nargs='?', help='a disk image')
parser.add_argument('--folder', default=None, nargs='?', help='a folder with files')
parser.add_argument('--file', default="memdisk.asm", help='the output assembly file')
args = parser.parse_args()

f = open(args.file, "w")

f.write("USE32\nALIGN 4096\n")
f.write("section .diskdata\n")
if args.disk:
  # write image and close disk
  f.write('   incbin "' + args.disk + '"\n')
  f.close()
  # if a folder is specified, lets assume the disk is
  # to be created from the contents of a folder:
  if args.folder:
    image = args.disk

    if _platform == "darwin":
      osx_dmg = args.disk + ".dmg"
      call(["hdiutil", "create", "-fs", "MS-DOS", "-o", args.disk, "-srcfolder", args.folder])
      print ("Renaming " + osx_dmg + " => " + image)
      call(["mv", osx_dmg, image])

    else:
      print ("Using folder: \t" + args.folder + "\nImage name: \t" + image)
      sectors = 16500 # Minimum for FAT16 (otherwise its formatted as FAT12)
      call(["dd", "if=/dev/zero", "of=" + args.disk, "count=" + str(sectors)])
      call(["mkfs.fat", image])

    exit(0)
