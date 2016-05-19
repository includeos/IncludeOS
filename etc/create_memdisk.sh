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
MIN_BLOCKS=34
MIN_ROOT_ENTS=16
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
echo -e "\n>>> Memdisk requires $size blocks, minimum block-count is $MIN_BLOCKS"

[ $size -gt $MIN_BLOCKS ] || size=$MIN_BLOCKS

echo -e ">>> Creating $size blocks"

root_entries=`ls -l memdisk | wc -l`
[ $root_entries -gt $MIN_ROOT_ENTS ] || root_entries=$MIN_ROOT_ENTS

echo -e ">>> Creating $root_entries root entries"

[ -e $FILENAME ] && rm $FILENAME

mkfs.fat -C $FILENAME -S $BLOCK_SIZE $size -n "INC_MEMDISK" -r $root_entries

mkdir -p $MOUNT
sudo mount $FILENAME $MOUNT
sudo cp -r memdisk/* $MOUNT/
sync
sudo umount $MOUNT
rmdir $MOUNT
