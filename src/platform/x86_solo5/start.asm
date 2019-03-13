;; This file is a part of the IncludeOS unikernel - www.includeos.org
;;
;; Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
;; and Alfred Bratterud
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http:;;www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

global get_fs_sentinel_value:function
global pre_initialize_tls:function
extern _ELF_START_
extern kernel_start
%define IA32_FS_BASE            0xc0000100

get_fs_sentinel_value:
    mov rax, [fs:0x28]
    ret

pre_initialize_tls:
    mov ecx, IA32_FS_BASE
    mov edx, 0x0
    mov eax, initial_tls_table
    wrmsr
    ;; stack starts at ELF boundary growing down
    mov rsp, _ELF_START_
    call kernel_start
    ret

SECTION .data
initial_tls_table:
    dd initial_tls_table
    dd 0
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0x123456789ABCDEF
    dq 0x123456789ABCDEF
