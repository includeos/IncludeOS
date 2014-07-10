all:
	nasm -f bin -o microMachine.bin microMachine.asm
	dd conv=notrunc if=microMachine.bin of=microMachine.hda
