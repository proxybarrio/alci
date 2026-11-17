//------------------------------------------------------------------------------
// #include <string.h>
// void * memchr (const void * s, int c, size_t n);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

	.global memchr

memchr:
	push %bp
	mov %sp,%bp

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov 4+FAR_ADJ_(%bp),%ax  // s
	mov 6+FAR_ADJ_(%bp),%dx  // c
	mov 8+FAR_ADJ_(%bp),%cx  // n
#endif
			     // regparmcall: s = AX, c = DX, n = CX

	// Save DI ES

	mov %di,%bx
	push %es
	push %ds
	pop %es

	// Do the scan

	mov %ax,%di
	mov %dl,%al
	xor %dx,%dx		// not found: return NULL
	jcxz 1f
	cld
	repne
	scasb
	jne 1f
	lea -1(%di),%dx		// found: address of the byte
1:
	mov %dx,%ax

	// Restore DI ES

	pop %es
	mov %bx,%di

	pop %bp
	RET_(6)

//------------------------------------------------------------------------------
