;      Copyright (C) 1997-2004 Andrei Los.
;      This file is part of the lSwitcher source package.
;      lSwitcher is free software; you can redistribute it and/or modify
;      it under the terms of the GNU General Public License as published
;      by the Free Software Foundation, in version 2 as it comes in the
;      "COPYING" file of the lSwitcher main distribution.
;      This program is distributed in the hope that it will be useful,
;      but WITHOUT ANY WARRANTY; without even the implied warranty of
;      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;      GNU General Public License for more details.


.286
seg_a		segment byte public
		assume cs:seg_a, ds:seg_a

start:		jmp	End_Resident_Portion

installed		db	0
char_buf		db	'$'
pipe_handle		dw	0
old_int_08		dd	00000h
old_int_09		dd	00000h
old_int_16		dd	00000h
old_int_28		dd	00000h
old_int_21		dd	00000h
critical_addr		dd	00000h
now_working		db	0
was_tab_release 	db	0
was_E0_prefix		db	0
right_down		db	0
left_down		db	0
use_ctrl		db	0
use_win			db	0
make_scan		db	38h
break_scan		db	0B8h
tab_down		db	0
sem_popupname		db	'\SEM32\LSWITCH\LSWITCH.S04',0

sem_shiftname		db	'\SEM32\LSWITCH\LSWITCH.S06',0
sem_ctrltabname 	db	'\SEM32\LSWITCH\LSWITCH.S08',0
sem_wintabname 		db	'\SEM32\LSWITCH\LSWITCH.S09',0
sem_handle		dd	00000h
post_count		dd	00000h
counter 		dd	00000h

int_08h_entry	proc	far
		pushf
		call	cs:old_int_08
		cli
		push	bx
		push	es
		les	bx,cs:critical_addr
; check the CriticalError and InDos (which follows the former) bytes
		cmp	word ptr es:[bx],0
		pop	es
		pop	bx
		jnz	exit_08

		cmp	cs:now_working,1
		je	exit_08
		cmp	cs:was_tab_release,1
		jne	exit_08
work_08:	mov	cs:now_working,1
		mov	cs:was_tab_release,0
		call	sub_1
		mov	cs:now_working,0
exit_08:
		sti
		iret
int_08h_entry	endp


int_16h_entry	proc	far
		push	bx
		push	es
		les	bx,cs:critical_addr
; check the CriticalError and InDos (which follows the former) bytes
		cmp	word ptr es:[bx],0
		pop	es
		pop	bx
		jnz	exit_16
		
		call	check_hotkey

exit_16:	jmp	cs:old_int_16
int_16h_entry	endp


int_21h_entry	proc	far
		cmp	ax,0F66Fh
		jne	exit_21
		cmp	bx,20080
		jne	exit_21
		mov	byte ptr ds:[si],1
exit_21:	jmp	cs:old_int_21
int_21h_entry	endp


int_28h_entry	proc	far
		pushf
		call	cs:old_int_28
		push	bx
		push	es
		les	bx,cs:critical_addr
; check the InDos byte for a value >1
		cmp	byte ptr es:[bx+1],1
		pop	es
		pop	bx
		ja	exit_28

		call	check_hotkey
		
		cmp	cs:now_working,1
		je	exit_28
		cmp	cs:was_tab_release,1
		jne	exit_28
work_28:	mov	cs:now_working,1
		mov	cs:was_tab_release,0
		call	sub_1
		mov	cs:now_working,0
exit_28:	iret
int_28h_entry	endp


int_09h_entry	proc	far
		push	ds
		push	ax
		mov	ax,cs
		mov	ds,ax
		cmp	cs:now_working,1
		je	old_09
		cmp	cs:was_tab_release,1
		je	old_09
		in	al,60h
		cmp	al,0E0h
		jne	not_prefix
		mov	cs:was_E0_prefix,1
		jmp	old_09
not_prefix:
		cmp	cs:was_E0_prefix,1
		jne	wasnt_prefix
		mov	cs:was_E0_prefix,0
		cmp	al,cs:make_scan       ; right alt or ctrl (if used) press
		jne	test_up
		mov	cs:right_down,1
		jmp	old_09
test_up:	cmp	al,break_scan	      ; right alt or ctrl release
		jne	old_09
		mov	cs:right_down,0
		jmp	old_09
wasnt_prefix:
;due to a bug in OS/2, that first Alt press after switching to a DOS
;session does not set the corresponding BIOS flag, we can't just test
;kbd flags, need to test actual scan codes for left Alt/Ctrl
		cmp	al,make_scan
		jne	test_left_up
		mov	left_down,1
		jmp	old_09
test_left_up:	cmp	al,break_scan
		jne	test_tab
		mov	left_down,0
		jmp	old_09
test_tab:
		cmp	al,0Fh
		jne	test_tab_release
		mov	byte ptr tab_down,1
		jmp	test_flags
test_tab_release:
		cmp	al,8Fh			; tab release
		jne	old_09
		mov	byte ptr tab_down,0
test_flags:
		cmp	cs:left_down,1
		jne	old_09
		cmp	cs:right_down,1
		je	old_09
		cmp	cs:tab_down,1
		je	end_int 	; tab press while Alt/Ctrl down
					; trap it for compatibility with DN
		push	es
		mov	ax,40h
		mov	es,ax
		mov	ax,es:[0017h]
		pop	es
		cmp	byte ptr use_ctrl,1
		je	test_ctrl

		test	al,4			; either ctrl down
		jnz	old_09
		test	ah,0F5h 		; caps,num,scroll,etc down
		jnz	old_09
		jmp	hotkey_detected
test_ctrl:
		test	al,8			; either alt down
		jnz	old_09
		test	ah,0F6h 		; caps,num,scroll,etc down
		jnz	old_09
hotkey_detected:
		mov	cs:was_tab_release,1
		and	al,3
		mov	cs:char_buf,al
end_int:
		in	al,61h
		mov	ah,al
		or	al,80h
		out	61h,al
		xchg	ah,al
		out	61h,al
		mov	al,20h
		out	20h,al
		pop	ax
		pop	ds
		sti
		iret
old_09:
		pop	ax
		pop	ds
		jmp	cs:old_int_09
int_09h_entry	endp


check_hotkey	proc	near
		pusha
		push	ds
		push	es

		mov	ax,40h
		mov	es,ax
		mov	ax,word ptr es:[6ch]
		mov	bx,word ptr es:[6eh]
		mov	cx,word ptr cs:counter
		mov	dx,word ptr cs:counter+2
		mov	word ptr cs:counter,ax
		mov	word ptr cs:counter+2,bx

		cmp	bx,dx
		jg	check_semaphore
		add	cx,20
		cmp	ax,cx
		jl	exit_ctrl_test

check_semaphore:
		mov	ax,cs
		mov	ds,ax
		mov	es,ax

		mov	di,offset sem_ctrltabname
		call	DWNOPENEVENTSEM
		call	DWNQUERYEVENTSEM
		call	DWNCLOSEEVENTSEM

		cmp	word ptr post_count,0
		jne	do_use_ctrl

		mov	byte ptr use_ctrl,0
		mov	byte ptr make_scan,38h
		mov	byte ptr break_scan,0B8h
		jmp	exit_ctrl_test
do_use_ctrl:
		mov	byte ptr use_ctrl,1
		mov	byte ptr make_scan,1dh
		mov	byte ptr break_scan,9dh
exit_ctrl_test:
		pop	es
		pop	ds
		popa
		retn
check_hotkey	endp

sub_1		proc	near
		pusha
		push	ds
		push	es

		mov	ax,40h			; check that we are in text mode
		mov	es,ax
		cmp	byte ptr es:[49h],13h
		ja	exit

		mov	ax,cs
		mov	ds,ax
		mov	es,ax

		mov	di,offset sem_shiftname
		call	DWNOPENEVENTSEM
		test	byte ptr char_buf,3
		jz	reset_shift_sem
		call	DWNPOSTEVENTSEM
		jmp	close_shift_sem
reset_shift_sem:
		call	DWNRESETEVENTSEM
close_shift_sem:
		call	DWNCLOSEEVENTSEM

		mov	di,offset sem_popupname
		call	DWNOPENEVENTSEM
		call	DWNPOSTEVENTSEM
		call	DWNCLOSEEVENTSEM

exit:
		pop	es
		pop	ds
		popa
		retn
sub_1		endp

; Open an event semaphore.
DWNOPENEVENTSEM PROC NEAR
	   MOV	    AH,64H		;  Set up the parameter list:
	   MOV	    CX,636CH		;     Magic number - see docs.
	   MOV	    BX,0145H		;     Indicator for open event sem.
	   MOV	    SI,OFFSET sem_handle   ;	 Address of semaphore handle (HEV)
	   MOV	    sem_handle,0

	   INT	    21H 	; Issue the multiplex interrupt
	   JC	    SHORT $+4	; Skip clearing AX if carry
	   XOR	    AX,AX	; Return code zero if no error

	   RETN
DWNOPENEVENTSEM ENDP

DWNPOSTEVENTSEM PROC NEAR
	   MOV	    AH,64H	      ;  Set up the parameter list:
	   MOV	    CX,636CH	      ;     Magic number - see docs.
	   MOV	    BX,0148H	      ;     Indicator for post event sem.
	   MOV	    SI,WORD PTR sem_handle   ;	   Semaphore handle (HEV)
	   MOV	    DX,WORD PTR sem_handle+2 ;	     into DX:SI

	   INT	    21H 	; Issue the multiplex interrupt
	   JC	    SHORT $+4	; Skip clearing AX if carry
	   XOR	    AX,AX	; Return code zero if no error

	   RETN
DWNPOSTEVENTSEM ENDP

DWNRESETEVENTSEM PROC NEAR
	   MOV	    AH,64H	       ;  Set up the parameter list:
	   MOV	    CX,636CH	       ;     Magic number - see docs.
	   MOV	    BX,0147H	       ;     Indicator for reset event sem.
	   MOV	    SI,WORD PTR sem_handle   ;	   Semaphore handle (HEV)
	   MOV	    DX,WORD PTR sem_handle+2 ;	     into DX:SI
	   MOV	    DI,OFFSET post_count

	   INT	    21H 	; Issue the multiplex interrupt
	   JC	    SHORT $+4	; Skip clearing AX if carry
	   XOR	    AX,AX	; Return code zero if no error

	   RETN
DWNRESETEVENTSEM ENDP

DWNCLOSEEVENTSEM PROC NEAR
	   MOV	    AH,64H	      ;  Set up the parameter list:
	   MOV	    CX,636CH	      ;     Magic number - see docs.
	   MOV	    BX,0146H	      ;     Indicator for close event sem.
	   MOV	    SI,WORD PTR sem_handle   ;	   Semaphore handle (HEV)
	   MOV	    DX,WORD PTR sem_handle+2 ;	     into DX:SI

	   INT	    21H 	; Issue the multiplex interrupt
	   JC	    SHORT $+4	; Skip clearing AX if carry
	   XOR	    AX,AX	; Return code zero if no error

	   RETN
DWNCLOSEEVENTSEM ENDP

DWNQUERYEVENTSEM PROC NEAR
	   MOV	    AH,64H	       ;  Set up the parameter list:
	   MOV	    CX,636CH	       ;     Magic number - see docs.
	   MOV	    BX,014AH	       ;     Indicator for query event sem.
	   MOV	    SI,WORD PTR sem_handle   ;	   Semaphore handle (HEV)
	   MOV	    DX,WORD PTR sem_handle+2 ;	     into DX:SI
	   MOV	    DI,OFFSET post_count

	   INT	    21H 	; Issue the multiplex interrupt
	   JC	    SHORT $+4	; Skip clearing AX if carry
	   XOR	    AX,AX	; Return code zero if no error

	   RETN
DWNQUERYEVENTSEM ENDP


End_Resident_Portion:
		mov	ax,cs
		mov	ds,ax

; check we are not loaded high
		cmp	ax,0A000h
		jb	test_ver
		mov	dx,offset mbl
		jmp	disp_n_exit
; get DOS version; we need 3+ to be able to get CriticalErr flag address
test_ver:
		mov	ah,30h
		int	21h
		cmp	al,3
		jae	ver_ok
		mov	dx,offset icv
		jmp	disp_n_exit
ver_ok:
		mov	majver,al
		mov	minver,ah

test_resident:
		mov	ax,0F66Fh
		mov	bx,20080
		mov	si,offset installed
		int	21h
		cmp	byte ptr installed,1
		jne	install
		mov	dx,offset arl
		jmp	disp_n_exit

install:
; get the CriticalError flag address, it's followed by InDos flag
		cmp	byte ptr majver,4
		je	ver_4
		mov	ax,5D06h
		jmp	get_addr
ver_4:
		mov	ax,5D0Bh
get_addr:
		push	ds
		int	21h
		mov	ax,ds
		pop	ds
		jnc	got_address
		mov	dx,offset cga
		jmp	disp_n_exit
got_address:
		mov	word ptr critical_addr,si
		mov	word ptr critical_addr+2,ax

; save old vectors
		mov	ax,3508h
		int	21h
		mov	word ptr old_int_08,bx
		mov	word ptr old_int_08+2,es
		mov	ax,3509h
		int	21h
		mov	word ptr old_int_09,bx
		mov	word ptr old_int_09+2,es
		mov	ax,3516h
		int	21h
		mov	word ptr old_int_16,bx
		mov	word ptr old_int_16+2,es
		mov	ax,3528h
		int	21h
		mov	word ptr old_int_28,bx
		mov	word ptr old_int_28+2,es
		mov	ax,3521h
		int	21h
		mov	word ptr old_int_21,bx
		mov	word ptr old_int_21+2,es

; set vectors
		mov	ax,2508h
		mov	dx,offset int_08h_entry
		int	21h
		mov	ax,2509h
		mov	dx,offset int_09h_entry
		int	21h
		mov	ax,2516h
		mov	dx,offset int_16h_entry
		int	21h
		mov	ax,2528h
		mov	dx,offset int_28h_entry
		int	21h
		mov	ax,2521h
		mov	dx,offset int_21h_entry
		int	21h

		mov	dx,offset End_Resident_Portion+100h
		int	27h

disp_n_exit:
		mov	ah,09
		int	21h
		mov	ax,4C00h
		int	21h


majver		db	0
minver		db	0
arl		db	'already loaded',13,10,'$'
;Czech
;arl		db	'lSwitcher uз b╪зб',13,10,'$'
;Italian
;arl		db	'Gia'' caricato',13,10,'$'
;Spanish
;arl		db	'Ya cargado',13,10,'$'
;arl		db	'Программа уже загружена',13,10,'$'
;German		
;arl		db	'Bereits gestartet',13,10,'$'
mbl		db	'this program cannot be loaded high',13,10,'$'
icv		db	'DOS version 3+ needed',13,10,'$'
cga		db	'cannot get CriticalError flag address',13,10,'$'

copyr		db	'lSwitcher V2.70 VDM TSR Copyright (C) 1997-2004 Andrei Los'

seg_a		ends
end	start
