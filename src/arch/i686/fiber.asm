; This file is a part of the IncludeOS unikernel - www.includeos.org
;
; Copyright 2017 Oslo and Akershus University College of Applied Sciences
; and Alfred Bratterud
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
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
