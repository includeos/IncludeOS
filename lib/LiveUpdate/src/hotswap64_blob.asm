; Master thesis
; by Alf-Andre Walla 2016-2017
global hotswap64
global hotswap64_len

SECTION .text
hotswap64:
    incbin "hotswap64.bin"

hotswap64_len:
    dd $ - hotswap64
