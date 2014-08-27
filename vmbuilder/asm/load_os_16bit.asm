load_os:
	;; 	xchg bx,bx
	xor eax,eax

	;; Disk interrupt vars (register aliases
	%define b_DISK dl
	%define b_CYLN ch
	%define b_HEAD dh
	%define b_SECT cl
	%define b_SECT_CNT al
	%define b_SECT_OFS ah
	%define TARGET_PTR bx

	;; Disk iterrupt aliases
	%define CHUNKSIZE 63

	;; Load target address
	mov bx,_os_segment
	mov es,bx	
	mov TARGET_PTR,_os_pointer

	;; Load total number of sectors
	;; 	xchg bx,bx
	xor si,si
	mov si,[srv_size]
	

	;; Load 0x13 call params
	mov b_DISK,0x80		; Which disk
	mov b_CYLN,0	     	; Which Cylynder
	mov b_HEAD,0		; Which head
	mov b_SECT,2		; Sector to start load
	mov b_SECT_CNT,CHUNKSIZE; [srv_size]-> max 63 sectors (Bochs)
	mov b_SECT_OFS,2	; Sector to start read-> first after boot

.continue:
	int 0x13
	cmp ah,0
	je .success

.read_error:
	mov si,str_err_disk
	call printstr
	ret

.success:

	;; Decrement counter
	sub si,CHUNKSIZE

	;; IF <= 0; Done, 
	cmp si,0
	jle .done
	xchg bx,bx	
	;; Else update/continue
	add TARGET_PTR,CHUNKSIZE*512
	jmp .continue
.done:
	mov si,str_success
	call printstr
	ret
