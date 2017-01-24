# Test booting with GRUB

The test will create a disk image with a GRUB bootloader and a minimal IncludeOS service. The test only verifies that the service boots with GRUB andstarts successfully - nothing else.

NOTE: The script that creates the image will:
* Require sudo
* Try to install GRUB 2.0 (using apt so very ubuntu specific)
* Mount stuff in a created local folder
* Use a loopback device
