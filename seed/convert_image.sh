qemu-img convert -O vdi $1 $1.vdi
qemu-img convert -O qcow2 $1 $1.qcow2
qemu-img convert -O vmdk $1 $1.vmdk

