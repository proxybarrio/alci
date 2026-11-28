// boot_minix_asm.S: zone_load of the MINIX boot loader payload (ALCI)
// Linked after boot_minix.o, so they take room in the second sector.

	.arch	i8086, nojumps
	.code16
	.text

// void zone_load (zone_nr z, unsigned off, unsigned seg)
// Gather contiguous file blocks into one multi-sector read: reading one
// 1K block per call made the drive wait almost a full turn between blocks.
// z = 0 reads the pending run.

	.global zone_load
	.global r_n
zone_load:
	mov r_n,%bx
	or %bx,%bx
	jz zl_new
	push %ax
	sub r_zone,%ax
	cmp %bx,%ax
	pop %ax
	jne zl_flush
	incw r_n
	ret
zl_flush:
	push %ax
	push %dx
	push %cx
	pushw r_seg
	shl %bx
	mov %bx,%dx
	mov r_off,%cx
	mov r_zone,%ax
	call disk_read_blk
	pop %cx
	pop %dx
	pop %ax
zl_new:
	mov %ax,r_zone
	mov %dx,r_off
	mov %cx,r_seg
	cmp $1,%ax
	sbb %bx,%bx
	inc %bx
	mov %bx,r_n
	ret

	.bss
r_n:	.skip 2
r_zone:	.skip 2
r_off:	.skip 2
r_seg:	.skip 2
	.text


