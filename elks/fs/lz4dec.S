// LZ4 block decoder for compressed executables (ALCI)
//
// unsigned int lz4_decompress(seg_t src_seg, seg_t dst_seg,
//                             unsigned int src_len, unsigned int dst_len)
//
// Decodes the LZ4 block at src_seg:0 (src_len bytes) to dst_seg:0.
// Returns the number of bytes written, or 0xFFFF if the block is bad:
// output past dst_len, input past src_len, or a match offset of zero or
// pointing before the start of the output. Each segment is addressed
// with offsets 0..len, so a block may be decoded in place, with the
// source above the destination in memory, as long as the writer never
// overtakes the reader (the packer checks this, see elks-lz4.py).
//
// Literals and matches are copied with rep movsb, which is also correct
// for overlapping matches (offset shorter than the length).
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

next_token:
	cmp	%bx,%si			// end of input: done
	jae	done
	lodsb
	mov	%ax,%bp			// BP (low byte) = token
	mov	$4,%cl
	shr	%cl,%al
	xor	%ch,%ch
	mov	%al,%cl			// CX = literal length
	cmp	$15,%cx
	jne	1f
	call	ext_len			// add extension bytes to CX
1:
	jcxz	2f
	mov	%di,%ax			// output bounds: DI + CX <= DX
	add	%cx,%ax
	jc	bad
	cmp	%dx,%ax
	ja	bad
	mov	%si,%ax			// input bounds: SI + CX <= BX
	add	%cx,%ax
	jc	bad
	cmp	%bx,%ax
	ja	bad
	rep
	movsb				// literals DS:SI -> ES:DI
2:
	cmp	%bx,%si			// last sequence has no match
	jae	done
	mov	%si,%ax			// two offset bytes must be there
	add	$2,%ax
	jc	bad
	cmp	%bx,%ax
	ja	bad
	mov	%bp,%cx			// CL = token
	lodsw				// AX = match offset
	or	%ax,%ax
	jz	bad
	cmp	%di,%ax			// offset <= bytes produced
	ja	bad
	mov	%ax,%bp			// BP = offset
	and	$15,%cx
	cmp	$15,%cx
	jne	3f
	call	ext_len
3:
	add	$4,%cx			// minimum match length
	jc	bad
	mov	%di,%ax			// output bounds: DI + CX <= DX
	add	%cx,%ax
	jc	bad
	cmp	%dx,%ax
	ja	bad
	push	%si
	push	%ds
	mov	%es,%ax
	mov	%ax,%ds
	mov	%di,%si
	sub	%bp,%si			// source = output - offset
	rep
	movsb
	pop	%ds
	pop	%si
	jmp	next_token

// CX += extension bytes (each 255 continues), with input bounds check
ext_len:
	cmp	%bx,%si
	jae	ext_bad
	lodsb
	xor	%ah,%ah
	add	%ax,%cx
	jc	ext_bad
	cmp	$255,%al
	je	ext_len
	ret
ext_bad:
	pop	%ax			// drop return address
	jmp	bad

bad:
	mov	$0xFFFF,%di
done:
	mov	%di,%ax
	pop	%ds
	pop	%es
	pop	%di
	pop	%si
	pop	%bp
	ret
