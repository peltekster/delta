; File implementing BMI2 decoding of bitpacked data. Used for the delta-encoding unpacking functions
;
; This is a temporary solution. As soon as the Apple's assembler starts supporting Haswell's instruction set, we'll
; revert to intrinsics or inline assembly.

section .data data align=16
upck:
	db 1
	db 2

; params: rdi - input data pointer
;         rsi - count of integers to be consumed (will be 32 currently, and will be divisible by 32 for sure)
;         rdx - output data pointer (uint32s)
;         rcx - number of bits (will be between 0 and 8 inclusive)
; returns (via eax) - number of bytes consumed from input
section .text
global _unpack_internal_bmi2
	align 16
_unpack_internal_bmi2:
	
	; create the mask:
	mov		r8,		1
	shl		r8,		cl
	dec 	r8

	; expand to 16 bits:
	mov		r9,		r8
	shl		r8,		8
	or		r8,		r9
	; expand to 32 bits:
	mov		r9,		r8
	shl		r8,		16
	or		r8,		r9
	; expand to 64 bits:
	mov		r9,		r8
	shl		r8,		32
	or		r8,		r9

	; mask ready in r8:
	pxor	xmm10,	xmm10

	xor		eax,	eax

align 16
.eight_bit_loop:
	cmp		eax,	esi
	jnb		.end_loop

	mov		r9,		[rdi]
	add		rdi,	rcx

	; input data in r9. Do bit unpacking, results in r10:
	pdep	r10,	r9,		r8
	vmovq	xmm0,	r10

	vpunpcklbw	ymm0, ymm0, ymm10 ; extend to 16-bit
	vpunpcklwd	ymm0, ymm0, ymm10 ; extend to 32-bit

	vextracti128	[rdx],	ymm0, 0
	vextracti128    [rdx+16], ymm0, 1

	add		rdx,	32 ; 8 ints * 32bits
	add		eax,	8   ; 8 ints processed
	jmp		.eight_bit_loop
.end_loop:		

	; bytes consumed = bits * count / 8
	mov		eax,	esi
	imul	rcx
	shr		eax,	3

	ret
