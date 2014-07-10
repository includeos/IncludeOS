;;; PIT Flags, bit 7 - bit 0
;;; bit:  [ 7,6  | 5,4    | 3,2,1 | 0 ]
;;; flag: [ chan | access | mode  | 0 ]
;;; Full description: http://wiki.osdev.org/Programmable_Interval_Timer#I.2FO_Ports
%define _pit_oneShot 00110000b	;Chan: 0, Access: lo/hi, Mode: 0, Bin/BCD: Bin
%define _pit_rateGen 00110100b	;Chan: 0, Access: lo/hi, Mode: 2, Bin/BCD: Bin
;;; PIT channels
%define _pit_chan0 40h
%define _pit_chan1 41h
%define _pit_chan2 42h
%define _pit_modeChan 43h

set_pit_mode:
	out _pit_modeChan, al
	ret
	
set_pit_value:
	mov al,1	;255 max
	mov ah,0	;255 max
	out 0x40,al     ;Set low byte of PIT reload value
	mov al,ah	
	out 0x40,al	;Send to PIT channel 0
	ret
