DISK=${DISK-grub_disk.img}
KERNEL=${KERNEL-build/test_grub}
MOUNTDIR=${MOUNTDIR-./mount}
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
  echo ">>> Copying kernel to $MOUNTDIR/boot"
  sudo cp $KERNEL $MOUNTDIR/boot
}

function build {
  echo ">>> Building service"
  pushd build
  make
  popd

}


# Unmount and Clean previous image
if [[ "$1" =~ -c.* ]]
then

  unmount
  clean

  if [[ "$1" =~ -ce.* ]]
  then
    echo "Exit option set. Exiting."
    exit 0
  fi
fi

# Update image and exit
if [[ "$1" == --update ]]
then
  mount_loopback
  pushd build
  make
  popd
  copy_kernel
  unmount
  exit
fi


# Exit on first error
set -e


# Default behavior

# FIXME: Install grub if needed
# NOTE: Done to give Jenkins the Grub tools
sudo apt install -y grub2

create_disk
mount_loopback

echo -e ">>> Creating boot dir"
sudo mkdir -p $MOUNTDIR/boot

echo -e ">>> Populating boot dir with grub config"
sudo cp -r grub $MOUNTDIR/boot
copy_kernel

echo -e ">>> Running grub install"
sudo grub-install --force --boot-directory $MOUNTDIR/boot/ $LOOP

unmount

echo -e ">>> Done. You can now boot $DISK"
