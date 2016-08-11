# MemDisk Test Service

Test to see that MemDisk is working correctly - also verifying the disk interface.

This test composed of 3 separate services:

1. Test a memdisk with 0 sectors (`sector0.disk`)
 - Verifies that the disk is empty.
2. Test a memdisk with 2 sectors (`sector2.disk`)
 - Verifies that the disk is 2 sectors.
 - Verifies reading and comparing binary data.
3. Test a big memdisk of 256 000 sectors (created by `bigdisk.sh`)
 - Verifies that a big disk is correctly included.
 - Verifies MBR (Master Boot Record) signature.
