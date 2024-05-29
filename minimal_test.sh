BIN=hello_includeos.elf.bin

# Halt on any error
set -e

# Build, IncludeOS + example bootable
nix-build example.nix

# Copy out from nix result and strip
cp ./result/bin/$BIN ./$BIN
chmod 755 ./$BIN
strip --strip-all $BIN

if command -v boot &> /dev/null
then
    echo "The 'boot' command is available."
    boot --create-bridge -g -d $BIN

else
    echo "The 'boot' command is not available."

    # Attach grub bootloader (requires sudo and "grub-pc" installed)
    ./vmbuild/grubify.sh $BIN


    # All of the below commands were tested to work on the following 2 platforms:

    # 1) Emulated:
    # AMD EPYC Processor (VM in openstack, emulated, Ubuntu 22.04)
    # QEMU emulator version 6.2.0 (Debian 1:6.2+dfsg-2ubuntu6.19)

    # qemu-system-x86_64 -nographic -hda hello_includeos.elf.bin.grub.img

    # 2) KVM hardware accelerated
    # Intel(R) Core(TM) i7-9700F CPU @ 3.00GHz (Alfred's desktop PC with ubuntu 24.04 with kvm)
    # QEMU emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1)

    # Basic kvm, throws a warning
    # sudo qemu-system-x86_64 --enable-kvm  -nographic -hda hello_includeos.elf.bin.grub.img

    # Enabling SMP, more CPU features
    sudo qemu-system-x86_64 --enable-kvm -smp 4 -cpu host,avx=on,xsave=on,aes=on -nographic -drive file=hello_includeos.elf.bin.grub.img,format=raw,index=0,media=disk

fi
