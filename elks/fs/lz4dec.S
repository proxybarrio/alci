// LZ4 block decoder for compressed executables (ALCI)
//
// unsigned int lz4_decompress(seg_t src_seg, seg_t dst_seg,
//                             unsigned int src_len, unsigned int dst_len)
//
// Decodes the LZ4 block at src_seg:0 (src_len bytes) to dst_seg:0.
// Returns the number of bytes written, or 0xFFFF if the block does not
// end exactly at src_len (or a length overflows). Each segment is addressed
// with offsets 0..len, so a block may be decoded in place, with the
// source above the destination in memory, as long as the writer never
// overtakes the reader (the packer checks this, see elks-lz4.py).
//
// Runs of 8 bytes or more are copied by words; a match whose source is
// only one byte back (a repeated byte) is copied by bytes, which rep movsb
// does correctly while the copy overlaps itself.
// Assumes DS=SS on entry, saves SI, DI, BP and ES, as the other far
// memory routines do.

#define ARG0	2
#define ARG1	4
#define ARG2	6
#define ARG3	8

	.arch	i8086, nojumps
	.code16
	.text

	.global	lz4_decompress

lz4_decompress:
	push	%bp
	push	%si
	push	%di
	push	%es
	push	%ds
	mov	%sp,%bp
	mov	10+ARG1(%bp),%es	// dst_seg
	mov	10+ARG2(%bp),%bx	// BX = src_len (end of input)
	mov	10+ARG3(%bp),%dx	// DX = dst_len (end of output)
	mov	10+ARG0(%bp),%ds	// src_seg (DS no longer kernel's)
	xor	%si,%si
	xor	%di,%di
	cld

// No checks per sequence: an executable is trusted code in real mode, and
// the disk's own CRC catches damaged sectors. The result must still end
// the input exactly, and the caller rejects one of the wrong length.
next_token:
	cmp	%bx,%si			// end of input: done
	jae	done
	lodsb
	mov	%ax,%bp			// BP (low byte) = token
	mov	$4,%cl
	shr	%cl,%al			// AL = literal length, ZF if none
	jz	2f
	cbw
	xchg	%ax,%cx			// CX = literal length
	cmp	$15,%cl
	jne	1f
	call	ext_len
1:
	cmp	$8,%cx
	jb	4f
	shr	$1,%cx			// long run: words, then the odd byte
	rep
	movsw
	adc	%cx,%cx
4:	rep
	movsb				// literals DS:SI -> ES:DI
2:
	cmp	%bx,%si			// last sequence has no match
	jae	done
	lodsw				// AX = match offset
	xchg	%ax,%bp			// BP = offset, AX = token
	and	$15,%ax
	xchg	%ax,%cx			// CX = match length - 4
	cmp	$15,%cl
	jne	3f
	call	ext_len
3:
	add	$4,%cx			// minimum match length
	push	%ds
	mov	%si,%ax			// save SI
	mov	%es,%si
	mov	%si,%ds			// match source is in the output
	mov	%di,%si
	sub	%bp,%si			// source = output - offset
	cmp	$2,%bp			// words when the source is 2+ bytes back
	jb	5f
	cmp	$8,%cx
	jb	5f
	shr	$1,%cx
	rep
	movsw
	adc	%cx,%cx
5:	rep
	movsb
	mov	%ax,%si
	pop	%ds
	jmp	next_token

// CX += extension bytes (each 255 continues); overflow is an error
ext_len:
	lodsb
	add	%al,%cl
	adc	$0,%ch
	jc	ext_bad
	cmp	$255,%al
	je	ext_len
	ret
ext_bad:
	pop	%ax			// drop return address
bad:
	mov	$0xFFFF,%di
	jmp	9f
done:
	cmp	%bx,%si			// input must end exactly
	jne	bad
9:
	mov	%di,%ax
	pop	%ds
	pop	%es
	pop	%di
	pop	%si
	pop	%bp
	ret
