//
// Low level routines for ELKS EtherLink III ethernet driver (3c509)
//
// Helge Skrivervik (@mellvik) june 2022
//

#include <linuxmt/config.h>
#include "arch/ports.h"
#include <arch/asm-offsets.h>

	.code16

#ifdef CONFIG_FARTEXT_KERNEL
// ALCI: the code is far, in its own section, which is released at boot when
// no card is found. Functions return far and their arguments start 2 bytes
// further up.
#define FARG	2
#define RETF	lret
#define EL3_TEXT .section .fartext.drv.el3,"ax",@progbits
#else
#define FARG	0
#define RETF	ret
#define EL3_TEXT .text
#endif

	.data
	.extern current


	EL3_TEXT

//-----------------------------------------------------------------------------
// Read data from port
//-----------------------------------------------------------------------------
// void el3_insw(int port, char *data, int count)
//

	.global el3_insw

el3_insw:

	push    %di
	mov	%sp,%di
	push	%es

	mov	4+FARG(%di),%dx	// Port
	mov	8+FARG(%di),%cx	// length (words)
	mov	6+FARG(%di),%di	// Buffer pointer

        mov     current,%bx
        mov     TASK_USER_DS(%bx),%es	// destination segment = user process
	cli

word_loop:
        in      %dx,%ax
        stosw
        loop    word_loop

	sti
        pop     %es
        pop     %di

        RETF


//-----------------------------------------------------------------------------
// Write data to port
//-----------------------------------------------------------------------------
// void el3_sendpk(int port, char *data, int count)
//

	.global el3_sendpk

el3_sendpk:
        push    %si
	mov	%sp,%si
	push	%ds

	mov	4+FARG(%si),%dx	// Port
        mov     8+FARG(%si),%cx      // byte count
        mov     6+FARG(%si),%si      // buffer addr

	cli
	mov	%cx,%ax
	outw	%ax,%dx		// send length
	xor	%ax,%ax
	outw	%ax,%dx		// dummy NULL word
	inc	%cx
	shr	%cx		// make word count
	inc	%cx
	and	$0xfffe,%cx	// make dword aligned
	mov     current,%bx	// setup for far memory xfer
	mov     TASK_USER_DS(%bx),%ds

wr_loop:
	lodsw
	out     %ax,%dx
	loop	wr_loop

	sti
	pop     %ds
	pop	%si

	RETF

//-------------------------------------------------
	.global el3_mdelay

el3_mdelay:

	push	%bp
	mov	%sp,%bp
	mov	4+FARG(%bp),%cx	// count
mdly_loop:
	outb	$0x80		// presumably approx 1us
	loop	mdly_loop

	pop	%bp
	RETF

