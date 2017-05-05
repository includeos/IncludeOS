;; =============================================================================
;;  ATA read sectors (LBA mode)
;; 
;;  Modification/rewrite of code found at osdev:
;;  http://wiki.osdev.org/ATA_read/write_sectors#Read_in_LBA_mode
;;
;; 	Licence: ...assumed to be in public domain 
;;
;;  @param EAX Logical Block Address of sector
;;  @param CL  Number of sectors to read
;;  @param EDI The address of buffer to put data obtained from disk
;;
;;  @return None
;; =============================================================================
ata_lba_read:
	pusha

	and eax, 0x0FFFFFFF
	
	mov ebx, eax ; Save LBA in RBX

	mov edx, 0x01F6 ; Port to send drive and bit 24 - 27 of LBA
	shr eax, 24     ; Get bit 24 - 27 in al
	or al, 11100000b	; Set bit 6 in al for LBA mode
	out dx, al
	
	mov edx, 0x01F2 ; Port to send number of sectors
	mov al, cl      ; Get number of sectors from CL
	out dx, al
	
	mov edx, 0x1F3 ; Port to send bit 0 - 7 of LBA
	mov eax, ebx   ; Get LBA from EBX
	out dx, al
	
	mov edx, 0x1F4 ; Port to send bit 8 - 15 of LBA
	mov eax, ebx   ; Get LBA from EBX
	shr eax, 8     ; Get bit 8 - 15 in AL
	out dx, al
	
	
	mov edx, 0x1F5 ; Port to send bit 16 - 23 of LBA
	mov eax, ebx   ; Get LBA from EBX
	shr eax, 16    ; Get bit 16 - 23 in AL
	out dx, al
	
	mov edx, 0x1F7 ; Command port
	mov al,  0x20  ; Read with retry.
	out dx, al

	;; Check for errors
	in al,dx

.drive_buffering:
	in al, dx
	test al, 8 		; the sector buffer requires servicing.
	jz .drive_buffering 	; until the sector buffer is ready.	
	test al, 64		; Drive executing command
	jz .drive_buffering	
	test al, 6
	jnz .drive_buffering
	
	test al,1		; There was an error
	jz .fetch_data	
	;; There was a read error...
	mov eax,0x00BADBAD
	cli
	hlt

.fetch_data:
	mov bl, cl    ; read CL sectors
	mov dx, 0x1F0 ; Data port, in and out
  ;;xor bx, bx
	;;mul bx
	mov  cx, 256   ; CX is counter for INSW
	rep insw	     ; read in to [EDI]
	in al, dx
	in al, dx
	in al, dx
	in al, dx
  
	popa
	ret
