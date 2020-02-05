global __apic_trampoline:function
extern __gdt64_base_pointer
extern revenant_main

;; we will be calling these from AP initialization
extern x86_enable_sse
extern x86_enable_fpu_native
extern x86_enable_xsave
extern x86_enable_avx

%define P4_TAB    0x1000
;; Extended Feature Enable Register (MSR)
%define IA32_EFER_MSR 0xC0000080
;; EFER Longmode bit
%define LONGMODE_ENABLE 0x100
;; EFER Execute Disable bit
%define NX_ENABLE 0x800
;; EFER Syscall enable bit
%define SYSCALL_ENABLE 0x1

[BITS 32]
__apic_trampoline:
	;; enable SSE before we enter C/C++ land
	call x86_enable_sse
	;; Enable modern x87 FPU exception handling
	call x86_enable_fpu_native
	;; try to enable XSAVE before checking AVX
	call x86_enable_xsave
	;; enable AVX if xsave and avx supported on CPU
	call x86_enable_avx

	pop edi  ;; cpuid

    ;; use same pagetable as CPU 0
    mov eax, P4_TAB
    mov cr3, eax

    ;; enable PAE
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

	;; enable long mode
    mov ecx, IA32_EFER_MSR
    rdmsr
    or  eax, (LONGMODE_ENABLE | NX_ENABLE | SYSCALL_ENABLE)
    wrmsr

    ;; enable paging
    mov eax, cr0                 ; Set the A-register to control register 0.
    or  eax, 1 << 31
    mov cr0, eax                 ; Set control register 0 to the A-register.

	;; retrieve CPU id -> rbx
    mov eax, 1
    cpuid
    shr ebx, 24
	;; TODO: load a proper GDT here instead of a shared one

    ;; load 64-bit GDT
    lgdt [__gdt64_base_pointer]
    jmp  0x8:long_mode ;; 0x8 = code seg

[BITS 64]
long_mode:
    ;; segment regs
    mov cx, 0x10 ;; 0x10 = data seg
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    mov ss, cx

    ;; align stack
    and  rsp, -16
    ;; geronimo!
    mov rdi, rbx
    call revenant_main
    ; stop execution
    cli
    hlt
