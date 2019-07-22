USE32
global __load_gdt:function

__load_gdt:
    mov  eax, [esp+4]
    lgdt [eax]
    jmp 0x8:reload_segments
reload_segments:
    mov	ax,  0x10
    mov	ds,  ax
    mov	es,  ax
    mov	fs,  ax
    mov	gs,  ax
    mov	ss,  ax
    ret
