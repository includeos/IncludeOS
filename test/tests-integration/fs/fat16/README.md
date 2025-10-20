# FAT16 Test Service

Test the functionalty of the FAT16 filesystem with the use of memdisk.

The test verifies the following things:

1. The disk size.
2. Auto mounting/recognition of filesystem.
3. Disk structure.
4. Disk content with the help of `banana.txt` and `banana.ascii`.
5. Filesystem API (read) and subclasses as `Dirent`.
6. Random access sync read.

`fat16_disk.sh` is used to create a 8 MB FAT16 disk `my.disk` with `banana.txt` as content, which will be included as a memdisk into the service.

