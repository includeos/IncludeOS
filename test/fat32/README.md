# FAT32 Test Service

Test the functionalty of the FAT32 filesystem with the use of VirioBlk disk.

The test verifies the following things:

1. The disk size.
2. Auto mounting/recognition of filesystem.
3. Disk structure.
4. Disk content with the help of `banana.txt` and `banana.ascii`.
5. Filesystem API (read) and subclasses as `Dirent`.
6. Remounting of filesystem.
7. Asynchronous operations (also nested like stat into read).

`fat32_disk.sh` is used to create a 4 GB FAT32 disk `my.disk` with `banana.txt` as content, which will be included by qemu as a separate drive.

