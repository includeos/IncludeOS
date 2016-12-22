DISK=${DISK-grub_disk.img}
KERNEL=${1-build/test_grub}
MOUNTDIR=${MOUNTDIR-/mnt}
MOUNT_OPTS="sync,rw"
BLOCKCCOUNT=10000


function unmount {

  # NOTE:  Mounted Loopback devices won't get detached with losetup -d
  # Instead we use umount -d to have mount detach the loopback device

  echo -e ">>> Unmounting and detaching $LOOP"
  sudo umount -vd $MOUNTDIR

}

function clean {
  echo ">>> Removing previous $DISK"
  rm -f $DISK
}


function create_disk {
  echo -e ">>> Creating FAT file system on $DISK with $BLOCKSIZE blocks"
  # NOTE: mkfs with the '-C' option creates the disk before creating FS
  # fallocate -l 10MiB $DISK

  mkfs.fat -C $DISK $BLOCKCCOUNT
}

function mount_loopback {

  # Find first available loopback device
  LOOP=$(losetup -f)

  echo -e ">>> Associating $LOOP with $DISK"
  # NOTE: To list used loopback devices do
  # losetup -a

  # Associate loopback with disk file
  sudo losetup $LOOP $DISK

  echo -e ">>> Mounting ($MOUNT_OPTS)  $DISK in $MOUNTDIR"
  mkdir -p $MOUNTDIR
  sudo mount -o $MOUNT_OPTS $LOOP $MOUNTDIR

}

function copy_kernel {
  echo ">>> Copying kernel '$KERNEL' to $MOUNTDIR/boot/includeos_service"
  sudo cp $KERNEL $MOUNTDIR/boot/includeos_service
}

function build {
  echo ">>> Building service"
  pushd build
  make
  popd
}



# Unmount and Clean previous image
if [[ "$2" =~ -c.* ]]
then

  unmount
  clean

  if [[ "$2" =~ -ce.* ]]
  then
    echo "Exit option set. Exiting."
    exit 0
  fi
fi

# Update image and exit
if [[ "$2" == --update ]]
then
  mount_loopback
  copy_kernel
  unmount
  exit
fi


# Exit on first error
set -e


# Default behavior
create_disk
mount_loopback

echo -e ">>> Creating boot dir"
sudo mkdir -p $MOUNTDIR/boot

GRUB_CFG='
set default="0"
set timeout=0
serial --unit=0 --speed=9600
terminal_input serial; terminal_output serial

menuentry IncludeOS Grub test {
  multiboot /boot/includeos_service
  }
'

if [ ! -e grub.cfg ]
then
  echo -e ">>> Creating grub config file"
  sudo echo "$GRUB_CFG" > grub.cfg
fi

echo -e ">>> Populating boot dir with grub config"
sudo mkdir -p $MOUNTDIR/boot/grub
sudo mv grub.cfg $MOUNTDIR/boot/grub/grub.cfg

  copy_kernel

echo -e ">>> Running grub install"
sudo grub-install --force --boot-directory $MOUNTDIR/boot/ $LOOP

unmount

echo -e ">>> Done. You can now boot $DISK"
