; This file is part of the svarlang project and is distributed onder the
; terms of the MIT license
;
; Copyright (C) 2024 Bernd Boeckmann
; 
; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the "Software"),
; to deal in the Software without restriction, including without limitation
; the rights to use, copy, modify, merge, publish, distribute, sublicense,
; and/or sell copies of the Software, and to permit persons to whom the
; Software is furnished to do so, subject to the following conditions:
; 
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
; DEALINGS IN THE SOFTWARE.
 
	EXTRN svarlang_mem:BYTE;
	EXTRN svarlang_dict:WORD;
	EXTRN svarlang_memsz:WORD;
	EXTRN svarlang_string_count:WORD;

.CODE

	PUBLIC	svarlang_strid
	PUBLIC	svarlang_load

;--------------------------------------------------------------------------
; svarlang_strid - return translation string identified by id
; performs a binary search of svarlang_dict
;--------------------------------------------------------------------------
; IN:  ax = string id
; OUT: ds:si -> string if found, otherwise si = zero
;--------------------------------------------------------------------------
svarlang_strid PROC
	push	bx
	push	cx
	push	dx

	xor	si,si			; si -> NULL string
	mov	cx,OFFSET svarlang_dict	; cx left search bounds into dict
	mov	dx,svarlang_string_count
	dec	dx
	shl	dx,1			; dx = dx * 4, as dict entry is
	shl	dx,1			; 4 bytes in size (id, offset)
	add	dx,cx			; dx right search bounds into dict
@@search:
	cmp	dx,cx			; right index < left index?
	 jb	@@not_found		;  then string id not found in dict
	mov	bx,dx			; bx = right
	sub	bx,cx			; bx = right - left
	shr	bx,1			; bx = (right - left) / 2
	and	bx,not 3		; take care of special case r-l=4
	add	bx,cx			; bx = left + (right - left) / 2
	cmp	ax,[bx]			; is it the searched dict id?
	 je	@@found
	 jb	@@below
@@above:				; current id is less then the one
	mov	cx,bx			; we search, continue search in
	add	cx,4			; right interval
	jmp	@@search
@@below:				; current id is greater than the one
	mov	dx,bx			; we search, continue searching left
	sub	dx,4			; interval
	jmp	@@search
@@found:
	mov	si,OFFSET svarlang_mem
	add	si,[bx+2]		; get string offset
@@not_found:
	pop	dx
	pop	cx
	pop	bx
	ret
svarlang_strid ENDP

;--------------------------------------------------------------------------
; svarlang_load - loads lang translations from file
;--------------------------------------------------------------------------
; IN:  ds:dx -> language file name
; OUT: flags: nc on success, cy on failure
;--------------------------------------------------------------------------
svarlang_load PROC
	ret
svarlang_load ENDP

	END
