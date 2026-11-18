ifndef randomInc
.data
randomInc equ 1
;for processors who don't support rdrand for whatever reason
DEFAULT_RAND_SEED_VALUE equ 67
RDSEED_CPUID_CODE equ 7h
RDSEED_SUPPORT_CHECK equ 40000h
RDSEED_MAX_TRY equ 1000
XORSHIFT_SHIFT_L_0 equ 13
XORSHIFT_SHIFT_R_1 equ 7
XORSHIFT_SHIFT_L_2 equ 17
;look at how this is used for an explanation on what it actually is
RAND_INT_SHIFT_FLOAT_SIGNIFICANCE equ 32

randSeed dq ?

.code

SeedRandom proc
	mov eax, RDSEED_CPUID_CODE
	xor ecx, ecx
	;if your processor doesn't support cpuid it wouldn't be able to run the rest of the program so i'm not going to check for that
	cpuid
	test ebx, RDSEED_SUPPORT_CHECK
	jnz rdseed_supported

	;;if your computer reaches this code your processor is cooked. (you're either running this on a raspberry pi or a potatoe)
	mov rax, DEFAULT_RAND_SEED_VALUE
	jmp breakSeedLoop

rdseed_supported:
	mov ecx, RDSEED_MAX_TRY
randSeedHead:
	rdseed rax
	jc breakSeedLoop
	loop randSeedHead
breakSeedLoop:
	mov qword ptr [randSeed], rax
	ret
SeedRandom endp

;https://en.wikipedia.org/wiki/Xorshift
FullRandom macro intGPR1, intGPR2, intGPR3, intGPR4, randSeedLoc

	mov intGPR1, qword ptr [randSeedLoc]
	mov intGPR2, intGPR1
	mov intGPR3, intGPR1
	mov intGPR4, intGPR1
	shl intGPR2, XORSHIFT_SHIFT_L_0
	shr intGPR3, XORSHIFT_SHIFT_R_1
	shl intGPR4, XORSHIFT_SHIFT_L_2
	xor intGPR1, intGPR2
	xor intGPR1, intGPR3
	xor intGPR1, intGPR4
	mov qword ptr [randSeedLoc], intGPR1
endm

;xmm0: min (signed), xmm1: max (signed). do not reverse order of xmm0 and xmm1.
RandFloat proc
	FullRandom rax, r8, r9, r10, randSeed

	;we shift by an arbitrary 32 bits right here because floats get less significant as they get larger, so if we scrap 32 bits of the random data and accept that the maximum value is 2^32-1, then divide by that value, most of the larger values of rax will give more accurate float representations.
	shr rax, RAND_INT_SHIFT_FLOAT_SIGNIFICANCE
	cvtsi2ss xmm3, rax
	divss xmm3, real4 ptr [u32_max_f]
	movups xmm2, xmm1
	subss xmm2, xmm0
	mulss xmm3, xmm2
	addss xmm3, xmm0
	movss xmm0, xmm3
	ret
RandFloat endp

;xmm0 min (signed), xmm1: max (signed). do not reverse order of xmm0 and xmm1.
RandVec proc
	push rbp
	mov rbp, rsp
	sub rsp, sizeof real4 * 2
	insertps xmm2, xmm0, INSERT_1_TO_0
	movss real4 ptr [rbp - sizeof real4], xmm2
	insertps xmm2, xmm1, INSERT_1_TO_0
	movss real4 ptr [rbp - sizeof real4 * 2], xmm2
	call RandFloat
	movss xmm1, real4 ptr [rbp - sizeof real4 * 2]
	movss real4 ptr [rbp - sizeof real4 * 2], xmm0
	movss xmm0, real4 ptr [rbp - sizeof real4]
	call RandFloat
	insertps xmm0, xmm0, INSERT_0_TO_1
	insertps xmm0, real4 ptr [rbp - sizeof real4 * 2], INSERT_0_TO_0
	mov rsp, rbp
	pop rbp
	ret
RandVec endp

;rcx: min (signed), rdx: max (signed). do not reverse order of rcx and rdx.
RandInt proc
	FullRandom rax, r8, r9, r10, randSeed

	mov r8, rdx
	sub r8, rcx
	xor rdx, rdx
	div r8
	mov rax, rdx
	add rax, rcx
	ret
RandInt endp
endif