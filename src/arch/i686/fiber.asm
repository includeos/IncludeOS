USE32

global __fiber_jumpstart
global __fiber_yield

extern panic

;; TODO: Implement __fiber_jumpstart / __fiber_yield for i686

;; Start a function in a new thread
;; @param 1: Stack pointer
;; @param 2: Delegate to call (via jumpstarter)
;; @param 3: Stack pointer save location
__fiber_jumpstart:

    push str_panic
    call panic

;; Yield to a started or yielded thread
;; @param 1 : new stack to switch to
;; @param 2 : Stack pointer save location
__fiber_yield:

    push str_panic
    call panic

str_panic: db "Fibers not yet implemented for i686",0
