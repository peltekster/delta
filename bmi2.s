; File implementing BMI2 decoding of bitpacked data. Used for the delta-encoding unpacking functions
;
; This is a temporary solution. As soon as the Apple's assembler starts supporting Haswell's instruction set, we'll
; revert to intrinsics or inline assembly.

section .data data align=16

pdepConstants:
	dq  0x0000000000000000
	dq	0x0101010101010101
	dq	0x0303030303030303
	dq  0x0707070707070707
	dq	0x0f0f0f0f0f0f0f0f
	dq	0x1f1f1f1f1f1f1f1f
	dq	0x3f3f3f3f3f3f3f3f
	dq	0x7f7f7f7f7f7f7f7f
	dq	0xffffffffffffffff

; params: rdi - input data pointer
;         rsi - hardcoded to 32 right now
;         rdx - output data pointer (uint32s)
;         rcx - number of bits (will be between 0 and 8 inclusive)
; returns (via eax) - number of bytes consumed from input
section .text
global _unpack_internal_bmi2
	align 16
_unpack_internal_bmi2:
	
	lea		r11,	[rel pdepConstants]
	mov		r8,		[r11 + rcx * 8]

	; mask ready in r8:
	pxor	xmm10,	xmm10

	mov		rax,	[rdi          ]
	mov		 r9,	[rdi + rcx    ]
	lea		rdi,	[rdi + rcx * 2]

	mov		r10,	[rdi          ]
	mov		r11,	[rdi + rcx    ]
	lea		rdi,	[rdi + rcx * 2]

	; input data in r11-r14. Do bit unpacking, results in the same regs
	pdep	rax,	rax,	r8
	pdep	 r9,	 r9,	r8
	pdep	r10,	r10,	r8
	pdep	r11,	r11,	r8
	vmovq	xmm1,	rax
	vmovq	xmm2,	r9
	vmovq	xmm3,	r10
	vmovq	xmm4,	r11

	punpcklbw	xmm1, xmm10 ; extend to 16-bit
	punpcklbw	xmm2, xmm10 ; extend to 16-bit
	punpcklbw	xmm3, xmm10 ; extend to 16-bit
	punpcklbw	xmm4, xmm10 ; extend to 16-bit

		; disabled AVX2 version... AVX2 makes it all very slow!
		;vpunpcklwd	ymm1, ymm1, ymm10 ; extend to 32-bit
		;vpunpcklwd	ymm2, ymm2, ymm10 ; extend to 32-bit
		;vpunpcklwd	ymm3, ymm3, ymm10 ; extend to 32-bit
		;vpunpcklwd	ymm4, ymm4, ymm10 ; extend to 32-bit

		;vmovaps		[rdx     ],	ymm1
		;vmovaps		[rdx + 32],	ymm2
		;vmovaps		[rdx + 64],	ymm3
		;vmovaps		[rdx + 96],	ymm4

	movaps		xmm5,	xmm1
	movaps		xmm6,	xmm2
	movaps		xmm7,	xmm3
	movaps		xmm8,	xmm4
	punpcklwd	xmm1,	xmm10
	punpcklwd	xmm2,	xmm10
	punpcklwd	xmm3,	xmm10
	punpcklwd	xmm4,	xmm10
	punpckhwd	xmm5,	xmm10
	punpckhwd	xmm6,	xmm10
	punpckhwd	xmm7,	xmm10
	punpckhwd	xmm8,	xmm10

	movaps		[rdx     ],	xmm1
	movaps		[rdx + 16],	xmm5
	movaps		[rdx + 32],	xmm2
	movaps		[rdx + 48],	xmm6
	movaps		[rdx + 64],	xmm3
	movaps		[rdx + 80],	xmm7
	movaps		[rdx + 96],	xmm4
	movaps		[rdx +112],	xmm8


	; bytes consumed = bits * count / 8; count is assumed to be 32, so bytes_consumed is bits * 4
	lea		eax,	[ecx * 4]

	ret
