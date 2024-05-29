#! /bin/bash

# GRUB uses roughly 4.6 Mb of disk
GRUB_KB=5000
BASE_KB=$GRUB_KB

usage() {
  echo "Usage: $0 [OPTIONS] BINARY" 1>&2
  echo ""
  echo "Create a bootable grub image for a multiboot compatible BINARY"
  echo ""
  echo "OPTIONS:"
  echo "-c Unmount and clean previous image, for consecutive development runs"
  echo "-e Exit after cleaning. Only makes sense in combination with -c"
  echo "-k keep mountpoint open for inspection - don't unmount"
  echo "-u update image with new kernel"
  echo "-s base image size, to which BINARY size will be added. Default $GRUB_KB Kb"
  exit 1
}

while getopts "cekus:" o; do
  case "${o}" in
    c)
      clean=1
      ;;
    e)
      exit_on_clean=1
      ;;
    k)
      keep_open=1
      ;;
    u)
      update=1
      ;;
    s)
      BASE_KB=$OPTARG
      ;;
    *)
      usage
      ;;
  esac
done
shift $((OPTIND-1))


[[ $1 ]] || usage

KERNEL=$1
DISK=$(basename ${DISK-$KERNEL.grub.img})
MOUNTDIR=${MOUNTDIR-/mnt}
MOUNT_OPTS="rw"

set -e

function unmount {

  # NOTE:  Mounted Loopback devices won't get detached with losetup -d
  # Instead we use umount -d to have mount detach the loopback device
  #
  # NOTE:
  # If the script is interrupted before getting to this step you'll end up with
  # lots of half-mounted loopback-devices after a while.
  # Unmount by consecutive calls to command below.

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
  FAT_KB=$(( ($KERN_KB + $BASE_KB) / 10 ))

  DISK_KB=$(( $KERN_KB + $BASE_KB + $FAT_KB ))

  echo ">>> Estimated disk size: $BASE_KB Kb Base size + $KERN_KB Kb kernel + $FAT_KB Kb FAT = $DISK_KB Kb"
  echo ">>> Creating FAT file system on $DISK"

  mkfs.fat -C $DISK $DISK_KB
}

function mount_loopback {

  # Find first available loopback device
  LOOP=$(sudo losetup -f)

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
if [[ $clean ]]
then
  echo ">>> Cleaning "
  unmount
  clean

  if [[ $exit_on_clean ]]
  then
    echo "Exit option set. Exiting."
    exit 0
  fi
fi

# Update image and exit
if [[ $update ]]
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


echo -e ">>> Populating boot dir with grub config"
sudo mkdir -p $MOUNTDIR/boot/grub


GRUB_CFG='
set default="0"
set timeout=0
serial --unit=0 --speed=9600
terminal_input serial; terminal_output serial

menuentry IncludeOS {
  multiboot /boot/includeos_service
  }
'

if [[ ! -e grub.cfg ]]
then
  echo -e ">>> Creating grub config file"
  sudo echo "$GRUB_CFG" > grub.cfg
  sudo mv grub.cfg $MOUNTDIR/boot/grub/grub.cfg
else
  echo -e ">>> Copying grub config file"
  sudo cp grub.cfg $MOUNTDIR/boot/grub/grub.cfg
fi

copy_kernel

# EFI?
sudo mkdir -p $MOUNTDIR/boot/efi


ARCH=${ARCH-i386}
TARGET=i386-pc
echo -e ">>> Running grub install for target $TARGET"
sudo grub-install --target=$TARGET --force --boot-directory $MOUNTDIR/boot/ $LOOP

echo -e ">>> Synchronize file cache"
sync

if [[ -z $keep_open ]]
then
  unmount
else
  echo -e ">>> NOTE: Keeping mountpoint open"
fi

echo -e ">>> Done. You can now boot $DISK"
