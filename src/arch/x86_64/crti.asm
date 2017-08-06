  global _init
  global _fini

  section .init
_init:
  push rbp
  mov  rbp, rsp


  section .fini
_fini:
  push rbp
  mov  rbp, rsp
