; Thanks to Maghsoud for paving the way to SMP!
; We are still calling them Revenants
org 0x10000
BITS 16
; 2-bytes of jmp instruction, but aligned to 4-bytes (!)
  JMP boot_code

ALIGN 4
; 20 bytes of addresses that we must modify
; before we can start this bootloader
revenant_main  dd  0x0
stack_base     dd  0x0
stack_size     dd  0

ALIGN 4
boot_code:
  ; disable interrupts
  cli

  ; segment descriptor table
  lgdt  [cs:gdtr]

  mov   edx, cr0  ; set bit 0 of CR0
  or    edx, 1    ; to enable protected mode
  mov   cr0, edx

  ; A small delay
  nop
  nop
  nop

  ; flush and enter protected mode
  JMP DWORD 0x08:protected_mode

;; Global descriptor table
gdtr:
  dw gdt32_end - gdt32 - 1
  dq gdt32
gdt32:
  ;; Entry 0x0: Null desriptor
  dq 0x0
  ;; Entry 0x8: Code segment
  dw 0xffff          ;Limit
  dw 0x0000          ;Base 15:00
  db 0x00            ;Base 23:16
  dw 0xcf9a          ;Flags
  db 0x00            ;Base 32:24
  ;; Entry 0x10: Data segment
  dw 0xffff          ;Limit
  dw 0x0000          ;Base 15:00
  db 0x00            ;Base 23:16
  dw 0xcf92          ;Flags
  db 0x00            ;Base 32:24
gdt32_end:

BITS 32
protected_mode:
  cld

  ; set most segments to data (0x10)
  mov   ax, 0x10
  mov   ds, ax
  mov   es, ax
  mov   ss, ax

  ; retrieve CPU id
  mov eax, 1
  cpuid
  shr ebx, 24

  ; give separate stack to each cpu
  mov eax, DWORD [stack_size]
  mul ebx
  add eax, [stack_base]
  mov ebp, eax
  mov esp, ebp

  push  ebx
  call  [revenant_main]
  ; stop execution
  cli
  hlt
