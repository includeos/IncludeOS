.globl __clone_return
.globl __thread_yield
.globl __thread_restore
.extern __thread_suspend_and_yield

.section .text
__clone_return:
__thread_yield:
__thread_restore:
    ret
