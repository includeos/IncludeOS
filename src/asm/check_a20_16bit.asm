;;; Code from osdev:
;;; http://wiki.osdev.org/A20_Line#Testing_the_A20_line

;;; Licence: Assumed public domain

;;; A general disclaimer is provided:
	
;;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;;; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;;; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;;; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;;; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;;; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
;;; THE SOFTWARE.

;;; from  http://wiki.osdev.org/OSDev_Wiki:General_disclaimer

check_a20:
	    pushf
	    push ds
	    push es
	    push di
	    push si

	    cli

	    xor ax, ax 		; ax = 0
	    mov es, ax

	    not ax 		; ax = 0xFFFF
	    mov ds, ax

	    mov di, 0x0500
	    mov si, 0x0510

	    mov al, byte [es:di]
	    push ax

	    mov al, byte [ds:si]
	    push ax

	    mov byte [es:di], 0x00
	    mov byte [ds:si], 0xFF

	    cmp byte [es:di], 0xFF

	    pop ax
	    mov byte [ds:si], al

	    pop ax
	    mov byte [es:di], al

	    mov ax, 0
	    je check_a20__exit

	    mov ax, 1

check_a20__exit:
	    pop si
	    pop di
	    pop es
	    pop ds
	    popf

	    ret
