USE32
global __context_switch
extern __context_switch_delegate

section .text

__context_switch:
    ; retrieve stack location
    ;pop  ebx
    ; delegate reference
    ;pop  ecx
    
    ; create stack frame
    push ebp
    mov  ebp, esp
    
    ; save special purpose stuff
    push esi
    push edi

    ; save current ESP for later
    mov  eax, esp
    ; change to new stack
    mov  esp, ecx
    and  esp, 0xfffffff0
    ; room for arguments
    sub  esp, 16
    
    ; store old ESP on new stack
    mov  DWORD [esp  ], eax
    ; delegate
    mov  DWORD [esp+4], edx
    ; call function that can call delegates
    call __context_switch_delegate
    ; restore old stack
    mov  esp, [esp]

    ; restore special stuff
    pop  edi
    pop  esi

    ; return to origin
    pop  ebp
    ret
