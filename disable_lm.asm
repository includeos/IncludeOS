;
; Master thesis
; by Alf-Andre Walla 2016-2017
;
;
global disable_longmode
global longmode_winddown_len
extern _start

%define code32_segment 0x08
%define data32_segment 0x10

bootaddr:    dd  0
bootmagic:   dd  0
startaddr:   dd  0

[BITS 64]
SECTION .text
ALIGN 16
disable_longmode:
    cli
    mov [bootaddr],  edi ; a
    mov [bootmagic], esi ; b
    mov [startaddr], edx ; dst

    ; load 64-bit GDTR with 32-bit entries
    lgdt [gdtr64]
    ; enter compatibility mode
    mov ecx, ss
    push rcx
    push rsp
    pushf
    push code32_segment
    mov  ecx, compatibility_mode
    push rcx
    iretq

[BITS 32]
ALIGN 16
compatibility_mode:
    ; disable paging
    mov ecx, cr0
    and ecx, 0x7fffffff  ;; clear PG (bit 31)
    mov cr0, ecx
    ; disable LM
    mov ecx, 0xC0000080          ; EFER MSR
    rdmsr
    and eax, ~(1 << 8)           ; remove LM-bit
    wrmsr

    ;; enter 32-bit protected mode
    jmp code32_segment:protected_mode
protected_mode:
    mov cx, data32_segment
    mov ss, cx
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    ;; enter the new service from its entry point
    ;; in 32-bit protected mode, while passing
    ;; multiboot parameters in eax and ebx
    mov eax, [bootmagic]
    mov ebx, [bootaddr]
    jmp DWORD [startaddr]

gdtr:
    dw gdt32_end - gdt32 - 1
    dd gdt32
gdt32:
    ;; Entry 0x0: Null descriptor
    dq 0x0
    ;; Entry 0x18: 32-bit Code segment
    dw 0xffff          ;Limit
    dw 0x0000          ;Base 15:00
    db 0x00            ;Base 23:16
    dw 0xcf9a          ;Flags / Limit / Type [F,L,F,Type]
    db 0x00            ;Base 32:24
    ;; Entry 0x20: 32-bit Data segment
    dw 0xffff          ;Limit
    dw 0x0000          ;Base 15:00
    db 0x00            ;Base 23:16
    dw 0xcf92          ;Flags / Limit / Type [F,L,F,Type]
    db 0x00            ;Base 32:24
gdt32_end:
gdtr64:
    dw $ - gdt32 - 1   ; Limit
    dq gdt32           ; Base

longmode_winddown_len:
    dd $ - disable_longmode
