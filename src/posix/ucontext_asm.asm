USE32
extern errno

global getcontext
global setcontext
global restore_context_stack

%define EINVAL (22)
%define NULL (0)
%define sizeof_size_t (4)

struc ucontext_t
    .uc_link: resd 1
    .uc_stack: resb 12
    .uc_mcontext: resb 40 
endstruc

struc stack_t
    .ss_sp: resd 1
    .ss_size: resd 1
    .ss_flags: resd 1
endstruc

struc mcontext_t
    .edi: resd 1
    .esi: resd 1
    .ebp: resd 1
    .ebx: resd 1
    .edx: resd 1
    .ecx: resd 1
    .eax: resd 1
    
    .floating_point_env: resb 28
    .eip: resd 1
    .flags: resd 1 
    .ret_esp: resd 1
endstruc

section .text

;; =============================================================================
;;  Save the calling context into ucontext_t
;; 
;;  @param [ESP + 4] Pointer to a mcontext_t struct.
;;
;;  @return -1 if an error occured, 0 for success.
;; =============================================================================
getcontext:
    mov eax, DWORD [esp + 4]
    cmp eax, NULL
    jne .valid_arg
    mov eax, -1
    ret

.valid_arg:
    add eax, ucontext_t.uc_mcontext

    ;;General purpose registers
    mov DWORD [eax + mcontext_t.edi], edi
    mov DWORD [eax + mcontext_t.esi], esi
    mov DWORD [eax + mcontext_t.ecx], ecx
    mov DWORD [eax + mcontext_t.edx], edx
    mov DWORD [eax + mcontext_t.ebp], ebp
    mov DWORD [eax + mcontext_t.ebx], ebp

    ;; Floating point env
    fnstenv [eax + mcontext_t.floating_point_env]
    fldenv [eax + mcontext_t.floating_point_env]

    ;;Eip
    mov ecx, [esp]
    mov DWORD [eax + mcontext_t.eip], ecx

    ;;Return stack
    lea ecx, [esp + 4]
    mov DWORD [eax + mcontext_t.ret_esp], ecx

    ;;Flags
    pushfd
    mov ebx, [esp]
    mov DWORD [eax + mcontext_t.flags], ebx
    popfd

    sub eax, 12
    mov [eax + stack_t.ss_sp], ecx

    ;;Size doesn't matter ( ͡° ͜ʖ ͡°)
    mov DWORD [eax + stack_t.ss_size], 4096

    mov eax, 0

.return:
    ret

;; =============================================================================
;;  Switch to other context
;; 
;;  @param [ESP + 4] Pointer to a mcontext_t struct.
;;
;;  @return -1 if an error occured, doesn't return if successful.
;; =============================================================================
setcontext:
    mov eax, DWORD [esp + 4]
    test eax, eax

    jne .valid_arg
    
    mov eax, -1
    mov DWORD [errno], EINVAL
    ret

.valid_arg:
    mov eax, [esp + 4]
    add eax, 16 
    mov edi, [eax + mcontext_t.edi]
    mov esi, [eax + mcontext_t.esi]
    mov ebp, [eax + mcontext_t.ebp]
    mov ebx, [eax + mcontext_t.ebx]
    mov edx, [eax + mcontext_t.edx]
    mov ecx, [eax + mcontext_t.ecx]

    fldenv [eax + mcontext_t.floating_point_env]

    push DWORD [eax + mcontext_t.flags]
    popfd

    ;; New stack
    push DWORD [eax + mcontext_t.ret_esp]
    pop esp

    ;; Eip
    push DWORD [eax + mcontext_t.eip]

.return:
    ret

;; =============================================================================
;;  When a context finishes its execution, it will return to this function.
;;  This function will prepare the stack for 'setcontext', to successfuly
;;  return to the successor context.
;; 
;;  @param [ESP + 4] Returning context's argc
;;  @param [ESP + 8] - [ESP + 8 + (4 * argc)] Returning context's arguments
;;
;;  @return None
;; =============================================================================

restore_context_stack:
    ;; stack pointer
    lea ebx, [esp]

    ;;offset to the successor context pointer
    mov eax, sizeof_size_t
    mov ecx, DWORD [esp]
    mul ecx
    add ebx, eax
    add ebx, 4

    ;; successor context pointer
    mov eax, [ebx]
    mov [esp + 4], eax
    ;;Shouldn't return
    jmp setcontext 
