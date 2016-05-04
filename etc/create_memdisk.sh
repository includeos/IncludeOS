#! /bin/bash
#
# Stuff everything in the ./memdisk directory into a brand new
# FAT-formatted disk image, $FILENAME.
#
# WARNING:
# * Existing disk image of the same name gets overwritten
# * An assembly-file 'memdisk.asm' gets created / overwritten
#
# NOTE:
# * A temporary mount-point $MOUNT gets created then removed

if [ ! -d memdisk ]
then
    echo "Directory 'memdisk' not found"
    exit 1
fi

BLOCK_SIZE=512
FILENAME=memdisk.fat
MOUNT=temp_mount
ASM_FILE=memdisk.asm

# The assembly trick to get the memdisk into the ELF binary
cat > $ASM_FILE <<-EOF
USE32
ALIGN 32

section .diskdata
contents:
    incbin  "$FILENAME"

EOF

size=`du -s --block-size=$BLOCK_SIZE memdisk | cut -f1`
echo -e "\n>>> Creating memdisk with $size blocks"

[ -e $FILENAME ] && rm $FILENAME

mkfs.fat -C $FILENAME -S $BLOCK_SIZE $size

mkdir -p $MOUNT
sudo mount $FILENAME $MOUNT
sudo cp -r memdisk/* $MOUNT/
sync
sudo umount $MOUNT
rmdir $MOUNT
