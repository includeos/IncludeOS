; This file is a part of the IncludeOS unikernel - www.includeos.org
;
; Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

global __arch_start:function
extern kernel_start


[BITS 32]
__arch_start:
    pop eax
    pop ebx
    push eax
    push ebx
    call kernel_start
    ret
