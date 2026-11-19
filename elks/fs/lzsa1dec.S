//  LZSA1 block decoder for compressed executables (ALCI)
//
//  Adapted for the ELKS kernel (GNU as syntax, C calling convention, source
//  and destination segments as arguments, offsets 0) from lzsa1fta.asm, the
//  time-efficient 8088 decompressor of the LZSA project. This is an altered
//  version of the original. Original notice:
//
//  Copyright (C) 2019 Jim Leonard, Emmanuel Marty
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  unsigned int lzsa1_decompress(seg_t src_seg, seg_t dst_seg,
//                                unsigned int src_len, unsigned int dst_len)
//
//  Decodes the raw LZSA1 block at src_seg:0 to dst_seg:0 and returns the
//  number of bytes written. Like the original it trusts the block (an
//  executable is trusted code in real mode anyway); the caller rejects a
//  result of the wrong length. src_len and dst_len are not used.

#define ARG0	2
#define ARG1	4

	.arch	i8086, nojumps
	.code16
	.text

	.global	lzsa1_decompress

shr4table:
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
	.byte 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
	.byte 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
	.byte 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
	.byte 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
	.byte 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6
	.byte 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7

lzsa1_decompress:
	push	%bp
	push	%si
	push	%di
	push	%es
	push	%ds
	mov	%sp,%bp
	mov	10+ARG1(%bp),%es	// dst_seg
	mov	10+ARG0(%bp),%ds	// src_seg
	xor	%si,%si
	xor	%di,%di
	cld
	mov	$shr4table,%bx
	xor	%cx,%cx

decode_token:
	xchg	%cx,%ax			// clear AH (CX = 0 after a rep)
	lodsb				// token O|LLL|MMMM
	mov	%ax,%dx			// keep it in DL for MMMM

	and	$0x70,%al		// literal length (LLL)
	jz	check_offset_size
	cmp	$0x70,%al
	jne	got_literals

	lodsb				// extra length byte
	add	$7,%al
	jnc	got_literals_exact
	je	big_literals

mid_literals:
	lodsb				// 256 + one more byte
	inc	%ah
	xchg	%cx,%ax
	shr	$1,%cx
	rep
	movsw
	adc	$0,%cx
	rep
	movsb
	jmp	check_offset_size

big_literals:
	lodsw				// 16-bit length
	xchg	%cx,%ax
	shr	$1,%cx
	rep
	movsw
	adc	$0,%cx
	rep
	movsb
	jmp	check_offset_size

got_literals:
	.byte	0x2e			// CS: segment prefix
	xlat				// LLL >> 4 (table in CS)
got_literals_exact:
	xchg	%cx,%ax
	rep
	movsb

check_offset_size:
	test	%dl,%dl			// O bit: 16-bit offset
	js	get_long_offset
	mov	$0xff,%ah
	lodsb

get_match_length:
	xchg	%dx,%ax			// DX = offset (negative), AX = token
	and	$0x0f,%al		// MMMM
	cmp	$0x0f,%al
	jne	got_matchlen_short

	lodsb
	add	$0x12,%al		// MIN_MATCH_SIZE + MATCH_RUN_LEN
	jnc	do_long_copy
	jne	mid_matchlen

	lodsw				// 16-bit length, 0 = end of data
	xchg	%cx,%ax
	jcxz	done_decompressing
	jmp	copy_len_preset

get_long_offset:
	lodsw
	jmp	get_match_length

got_matchlen_short:
	add	$3,%al			// MIN_MATCH_SIZE
	xchg	%cx,%ax
	mov	%ds,%bp			// save DS
	mov	%es,%ax
	mov	%ax,%ds
	xchg	%ax,%si			// save SI
	mov	%di,%si
	add	%dx,%si			// back reference in the output
	rep
	movsb
	xchg	%ax,%si
	mov	%bp,%ds
	jmp	decode_token

done_decompressing:
	mov	%di,%ax			// bytes written (output started at 0)
	pop	%ds
	pop	%es
	pop	%di
	pop	%si
	pop	%bp
	ret

mid_matchlen:
	lodsb
	inc	%ah			// 256 + one more byte
do_long_copy:
	xchg	%cx,%ax
copy_len_preset:
	push	%ds
	mov	%es,%bp
	mov	%bp,%ds
	mov	%si,%bp			// save SI
	mov	%di,%si
	add	%dx,%si
	cmp	$-2,%dx			// byte or word run?
	jae	do_run
	shr	$1,%cx
	rep
	movsw
	adc	$0,%cx
	rep
	movsb
	mov	%bp,%si
	pop	%ds
	jmp	decode_token

do_run:
	je	do_run_2
do_run_1:
	lodsb				// one byte repeated
	mov	%al,%ah
	shr	$1,%cx
	rep
	stosw
	adc	$0,%cx
	rep
	stosb
	mov	%bp,%si
	pop	%ds
	jmp	decode_token

do_run_2:
	lodsw				// one word repeated
	shr	$1,%cx
	rep
	stosw
	adc	$0,%cx
	rep
	stosb
	mov	%bp,%si
	pop	%ds
	jmp	decode_token
