  global _init
  global _fini

  section .init
_init:
  push ebp
  mov DWORD ebp, esp


  section .fini
_fini:
  push ebp
  mov DWORD ebp, esp
