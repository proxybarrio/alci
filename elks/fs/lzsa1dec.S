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
//  number of bytes written, or 0 when the block is damaged; the caller
//  rejects any result but dst_len. src_len is not used.
//
//  Bounds (ALCI): a damaged block must not write outside the segment exec
//  allocated. Every sequence ends with a match of at least 3 bytes, so the
//  output grows at each one and the decoder always stops. Copies that can
//  be long (literals from 7 bytes, matches from 18) are checked against
//  dst_len (BX) before they start; after a short match the output is
//  checked. Short copies can pass dst_len by at most 6 + 17 bytes before
//  that check, inside the 32-byte minimum margin that exec allocates above
//  the output (FAST_MARGIN in exec.c). Reading past the
//  input is harmless and ends the same way. The original's table for
//  LLL >> 4 is replaced by a shift, which frees BX.

#define ARG0	2
#define ARG1	4
#define ARG3	8

// fail unless the CX bytes about to be written end at or before dst_len
.macro CHECKLEN bad
	mov	%di,%ax
	add	%cx,%ax
	jc	\bad
	cmp	%bx,%ax
	ja	\bad
.endm

	.arch	i8086, nojumps
	.code16
	.text

	.global	lzsa1_decompress

// the decoding loop, once for two segments (s = 2) and once for one (s = 1)
.macro DECODE s, one
decode_token\s:
	xchg	%cx,%ax			// clear AH (CX = 0 after a rep)
	lodsb				// token O|LLL|MMMM
	mov	%ax,%dx			// keep it in DL for MMMM

	and	$0x70,%al		// literal length (LLL)
	jz	check_offset_size\s
	cmp	$0x70,%al
	jne	got_literals\s

	lodsb				// extra length byte
	add	$7,%al
	jc	long_literals\s
	xchg	%cx,%ax			// 7 to 255 literals
	CHECKLEN damaged\s
	rep
	movsb
	jmp	check_offset_size\s

long_literals\s:
	je	big_literals\s
mid_literals\s:
	lodsb				// 256 + one more byte
	inc	%ah
	xchg	%cx,%ax
	CHECKLEN damaged\s
	shr	$1,%cx
	rep
	movsw
	adc	$0,%cx
	rep
	movsb
	jmp	check_offset_size\s

big_literals\s:
	lodsw				// 16-bit length
	xchg	%cx,%ax
	CHECKLEN damaged\s
	shr	$1,%cx
	rep
	movsw
	adc	$0,%cx
	rep
	movsb
	jmp	check_offset_size\s

got_literals\s:
	mov	$4,%cl			// 1 to 6 literals: LLL >> 4
	shr	%cl,%al
	xchg	%cx,%ax
	rep
	movsb

check_offset_size\s:
	test	%dl,%dl			// O bit: 16-bit offset
	js	get_long_offset\s
	mov	$0xff,%ah
	lodsb

get_match_length\s:
	xchg	%dx,%ax			// DX = offset (negative), AX = token
	and	$0x0f,%al		// MMMM
	cmp	$0x0f,%al
	jne	got_matchlen_short\s

	lodsb
	add	$0x12,%al		// MIN_MATCH_SIZE + MATCH_RUN_LEN
	jnc	do_long_copy\s
	jne	mid_matchlen\s

	lodsw				// 16-bit length, 0 = end of data
	xchg	%cx,%ax
	jcxz	done_decompressing\s
	jmp	copy_len_preset\s

get_long_offset\s:
	lodsw
	jmp	get_match_length\s

got_matchlen_short\s:
	add	$3,%al			// MIN_MATCH_SIZE
	xchg	%cx,%ax
.if \one == 0
	mov	%ds,%bp			// save DS
	mov	%es,%ax
	mov	%ax,%ds
.endif
	xchg	%ax,%si			// save SI
	mov	%di,%si
	add	%dx,%si			// back reference in the output
	rep
	movsb
	xchg	%ax,%si
.if \one == 0
	mov	%bp,%ds
.endif
	cmp	%bx,%di			// output past dst_len: damaged\s block
	ja	damaged\s
	jmp	decode_token\s

damaged\s:
	xor	%di,%di			// returns 0
done_decompressing\s:
	mov	%di,%ax			// bytes written (output started at 0)
	pop	%ds
	pop	%es
	pop	%di
	pop	%si
	pop	%bp
	ret

mid_matchlen\s:
	lodsb
	inc	%ah			// 256 + one more byte
do_long_copy\s:
	xchg	%cx,%ax
copy_len_preset\s:
	CHECKLEN damaged\s
.if \one == 0
	push	%ds
	mov	%es,%bp
	mov	%bp,%ds
.endif
	mov	%si,%bp			// save SI
	mov	%di,%si
	add	%dx,%si
	cmp	$-2,%dx			// byte or word run?
	jae	do_run\s
	shr	$1,%cx
	rep
	movsw
	adc	$0,%cx
	rep
	movsb
	mov	%bp,%si
.if \one == 0
	pop	%ds
.endif
	jmp	decode_token\s

do_run\s:
	je	do_run_2\s
do_run_1\s:
	lodsb				// one byte repeated
	mov	%al,%ah
	shr	$1,%cx
	rep
	stosw
	adc	$0,%cx
	rep
	stosb
	mov	%bp,%si
.if \one == 0
	pop	%ds
.endif
	jmp	decode_token\s

do_run_2\s:
	lodsw				// one word repeated
	shr	$1,%cx
	rep
	stosw
	adc	$0,%cx
	rep
	stosb
	mov	%bp,%si
.if \one == 0
	pop	%ds
.endif
	jmp	decode_token\s
.endm

lzsa1_decompress:
	push	%bp
	push	%si
	push	%di
	push	%es
	push	%ds
	mov	%sp,%bp
	mov	10+ARG1(%bp),%es	// dst_seg
	mov	10+ARG3(%bp),%bx	// dst_len
	mov	10+ARG0(%bp),%ds	// src_seg
	xor	%si,%si
	xor	%di,%di
	cld
	xor	%cx,%cx

	DECODE 2, 0

	.global	lzsa1_decompress1
//  unsigned int lzsa1_decompress1(seg_t seg, unsigned int src_off,
//                                 unsigned int src_len, unsigned int dst_len)
//
//  The same block, read from seg:src_off and written to seg:0 (ALCI): when
//  the whole in-place layout fits in one segment, DS = ES and a match
//  copies from the output without switching DS, which saves four segment
//  moves at each match (most sequences in program code are short matches).
lzsa1_decompress1:
	push	%bp
	push	%si
	push	%di
	push	%es
	push	%ds
	mov	%sp,%bp
	mov	10+ARG0(%bp),%es	// seg
	mov	10+ARG3(%bp),%bx	// dst_len
	mov	10+ARG1(%bp),%si	// src_off
	push	%es
	pop	%ds
	xor	%di,%di
	cld
	xor	%cx,%cx
	DECODE 1, 1
