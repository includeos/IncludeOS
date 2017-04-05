#! /bin/bash
KERNEL=${1-build/test_grub}
DISK=${DISK-$KERNEL.grub.img}
MOUNTDIR=${MOUNTDIR-/mnt}
MOUNT_OPTS="rw"

# GRUB uses roughly 4.6 Mb of disk
GRUB_KB=5000

set -e

function unmount {

  # NOTE:  Mounted Loopback devices won't get detached with losetup -d
  # Instead we use umount -d to have mount detach the loopback device

  echo -e ">>> Unmounting and detaching $LOOP"
  sudo umount -vd $MOUNTDIR || :

}

function clean {
  echo ">>> Removing previous $DISK"
  rm -f $DISK
}


function create_disk {

  if [ -f $DISK ]
  then
    echo -e ">>> $DISK allready exists. Preserving existing image as $DISK.bak"
    mv $DISK $DISK.bak
  fi

  # Kernel size in Kb
  KERN_KB=$(( ($(stat -c%s "$KERNEL") / 1024) ))

  # Estimate some overhead for the FAT
  FAT_KB=$(( ($KERN_KB + $GRUB_KB) / 10 ))

  DISK_KB=$(( $KERN_KB + $GRUB_KB + $FAT_KB ))

  echo ">>> Estimated disk size: $GRUB_KB Kb GRUB + $KERN_KB Kb kernel + $FAT_KB Kb FAT = $DISK_KB Kb"
  echo ">>> Creating FAT file system on $DISK"

  mkfs.fat -C $DISK $DISK_KB
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
  sync
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
sudo grub-install --target=i386-pc --force --boot-directory $MOUNTDIR/boot/ $LOOP

echo -e ">>> Synchronize file cache"
sync

unmount

echo -e ">>> Done. You can now boot $DISK"
