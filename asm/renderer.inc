ifndef renderer
renderer equ 1
.data
;minimum float value AS an integer
FLT_MIN_I equ -26172378
;minimum doubleword value AS a quadword
I32_MIN_Q equ 2147483648
;minimum doubleword value AS a doubleword
I32_MIN equ -2147483648
;minimum quadword value
I64_MIN equ -9223372036854775808
;a mask containing the minimum doubleword value AS a quadword and the minimum quadword value as a quadword
BOTH_I_MIN_MASK equ -9223372034707292160
;disregards z and w
SHUFFLE_CROSS_X_Y equ 1

HALF_ROTATION equ 127

INSERT_0_TO_0 equ 0h
INSERT_0_TO_1 equ 10h
INSERT_0_TO_2 equ 20h
INSERT_0_TO_3 equ 30h

INSERT_1_TO_0 equ 40h
INSERT_1_TO_1 equ 50h
INSERT_1_TO_2 equ 60h
INSERT_1_TO_3 equ 70h

INSERT_2_TO_0 equ 80h
INSERT_2_TO_1 equ 90h
INSERT_2_TO_2 equ 0a0h
INSERT_2_TO_3 equ 0b0h

INSERT_3_TO_0 equ 0c0h
INSERT_3_TO_1 equ 0d0h
INSERT_3_TO_2 equ 0e0h

;default cube veritces
cubeVerticesX real4 0.5,  0.5,  0.5,  0.5, -0.5, -0.5, -0.5, -0.5
cubeVerticesY real4 0.5,  0.5, -0.5, -0.5,  0.5,  0.5, -0.5, -0.5
cubeVerticesZ real4 0.5, -0.5, -0.5,  0.5,  0.5, -0.5, -0.5,  0.5
SingleDefVec verticesZOffset, -7.0
SingleDefVec halves, 0.5
SingledefVec zeroVec, 0.0
ONE_AT_1 real4 0.0, 1.0, 0.0, 0.0
NanVec dd -1, -1, -1, -1, -1, -1, -1, -1
PosNanVec dd 2147483647, 2147483647, 2147483647, 2147483647, 2147483647, 2147483647, 2147483647, 2147483647
NegNanVec dd 0FFFFFFFFh, 0FFFFFFFFh, 0FFFFFFFFh, 0FFFFFFFFh, 0FFFFFFFFh, 0FFFFFFFFh, 0FFFFFFFFh, 0FFFFFFFFh
rotationX db 0
rotationY db 0
.code

ConvertHorizonPtToVert macro MAX_RET_LOC, VERTEX_OFFSET
	RestoreVertices

	vmovups ymmword ptr [MAX_RET_LOC - sizeof ymmword], ymm4
	vmovups ymmword ptr [MAX_RET_LOC - sizeof ymmword * 2], ymm5
	vmovups ymmword ptr [MAX_RET_LOC - sizeof ymmword * 3], ymm6

	mov eax, dword ptr [MAX_RET_LOC - sizeof ymmword + VERTEX_OFFSET]
	mov real4 ptr [MAX_RET_LOC - sizeof ymmword * 4], eax;load x vertex from ymm4 into MAX_RET_LOC - sizeof ymmword * 4
	mov eax, dword ptr [MAX_RET_LOC - sizeof ymmword * 2 + VERTEX_OFFSET]
	mov real4 ptr [MAX_RET_LOC - sizeof ymmword * 4 + sizeof real4 * 1], eax;load y vertex from ymm5 into MAX_RET_LOC - sizeof ymmword * 4 + sizeof real4
	mov eax, dword ptr [MAX_RET_LOC - sizeof ymmword * 3 + VERTEX_OFFSET]
	mov real4 ptr [MAX_RET_LOC - sizeof ymmword * 4 + sizeof real4 * 2], eax;load z vertex from ymm6 into MAX_RET_LOC - sizeof ymmword * 4 + sizeof real4 * 2
	
	movups xmm1, xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4];now x vertex is in xmm1, y vertex is in xmm1 + sizeof real4, and z vertex is in xmm1 + sizeof real4 * 2, last real4 part is dummy memory.
endm

RotateVertices macro STORE_LOCATION, VERTEX_OFFSET_INDEX

	MAX_RET_LOC equ STORE_LOCATION - sizeof ymmword * 3
	
	;;;this initial rotation has two solutions. i have written both. i believe the quaternion one to be simpler so i will leave it in.
	;;;QUATERNION SOLUTION:
	;mov al, rotationX
	;movups xmm0, xmmword ptr [XAxis]
	;call QuatAngleAxis
	;movups xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4 - sizeof xmmword], xmm0
	;
	;mov al, rotationY
	;movups xmm0, xmmword ptr [YAxis]
	;call QuatAngleAxis
	;movups xmm1, xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4 - sizeof xmmword]
	;call QuatMult

	;;i'm using this one because it's simpler
	;;;EULER SOLUTION:
	mov al, rotationX
	mov cl, rotationY
	call EulerToQuat
	movups xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4 - sizeof xmmword], xmm0
	movups xmm0, xmmword ptr [XAxis]
	
	mov al, HALF_ROTATION
	call QuatAngleAxis
	movups xmm1, xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4 - sizeof xmmword]
	call QuatMult


	;movups xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4 - sizeof xmmword], xmm0
	;movups xmm1, xmmword ptr [AbsoluteRotation + VERTEX_OFFSET_INDEX * sizeof xmmword]
	;
	;call QuatMult
	;movups xmm1, xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4 - sizeof xmmword]
	;movups xmmword ptr [AbsoluteRotation + VERTEX_OFFSET_INDEX * sizeof xmmword], xmm1
	;call QuatMult

	ConvertHorizonPtToVert MAX_RET_LOC, VERTEX_OFFSET_INDEX * sizeof real4

	call QuatMultVec
	vmovups ymmword ptr [rsp], ymm0
	mov eax, dword ptr [rsp + sizeof real4 * 0]
	mov dword ptr [STORE_LOCATION + VERTEX_OFFSET_INDEX * sizeof real4 - sizeof ymmword * 1], eax
	mov eax, dword ptr [rsp + sizeof real4 * 1]
	mov dword ptr [STORE_LOCATION + VERTEX_OFFSET_INDEX * sizeof real4 - sizeof ymmword * 2], eax
	mov eax, dword ptr [rsp + sizeof real4 * 2]
	mov dword ptr [STORE_LOCATION + VERTEX_OFFSET_INDEX * sizeof real4 - sizeof ymmword * 3], eax
endm

RestoreDefaultPoints macro
	vmovups ymm4, ymmword ptr [cubeVerticesX]
	vmovups ymm5, ymmword ptr [cubeVerticesY]
	vmovups ymm6, ymmword ptr [cubeVerticesZ]
endm

RestoreVertices macro
	vmovups ymm4, ymmword ptr [rbp - sizeof ymmword * 1]
	vmovups ymm5, ymmword ptr [rbp - sizeof ymmword * 2]
	vmovups ymm6, ymmword ptr [rbp - sizeof ymmword * 3]
endm


RestorePoint macro STORE_LOCATION
	movups xmm0, xmmword ptr [STORE_LOCATION]
endm

;;;;what do i mean by vertical versus horizontal vertices? here is an example:
;;;;this is vertical:
;;;;xmm0: 0, 2, 4, 6
;;;;xmm1: 1, 3, 5, 7
;;;;and we want them horizontally sequential, like so:
;;;;ymm2: 0, 1, 2, 3, 4, 5, 6, 7
;;;;or even like so:
;;;;xmm2: 0, 1, 2, 3
;;;;xmm3: 4, 5, 6, 7
;;;;AND ANOTHER ONE
;;;;vertical:
;;;;ymm0: x0, x1, x2, x3, x4, x5, x6, x7
;;;;ymm1: y0, y1, y2, y3, y4, y5, y6, y7
;;;;to horizontal:
;;;;xmm2: x0 - x1, x2 - x3, x4 - x5, x6 -x7
;;;;xmm3: y0 - y1, y2 - y3, y4 - y5, y6 - y7

;loads first vertical vertices from xmmSrc1 and xmmSrc2 to xmmDst
LoadFirstVerticalVertices macro xmmDst, xmmSrc1, xmmSrc2
	movss xmmDst, xmmSrc1
	insertps xmmDst, xmmSrc2, INSERT_0_TO_1
endm

;loads first vertical vertices from xmmSrc1 and xmmSrc2 to xmmDst. INDEX 1, not first vertices.
LoadVerticalVertices1 macro xmmDst, xmmSrc1, xmmSrc2
	insertps xmmDst, xmmSrc1, INSERT_1_TO_0
	insertps xmmDst, xmmSrc2, INSERT_1_TO_1
endm

;loads first vertical vertices from xmmSrc1 and xmmSrc2 to xmmDst
LoadVerticalVertices2 macro xmmDst, xmmSrc1, xmmSrc2
	insertps xmmDst, xmmSrc1, INSERT_2_TO_0
	insertps xmmDst, xmmSrc2, INSERT_2_TO_1
endm

;loads first vertical vertices from xmmSrc1 and xmmSrc2 to xmmDst
LoadVerticalVertices3 macro xmmDst, xmmSrc1, xmmSrc2
	insertps xmmDst, xmmSrc1, INSERT_3_TO_0
	insertps xmmDst, xmmSrc2, INSERT_3_TO_1
endm

;uses ymmword of stack
LoadVerticalVerticesI macro xmmDst, ymmSrc1, ymmSrc2, i
	vmovups ymmword ptr [rbp - sizeof ymmword], ymmSrc1
	movss xmmDst, real4 ptr [rbp - sizeof ymmword + sizeof real4 * i]
	vmovups ymmword ptr [rbp - sizeof ymmword], ymmSrc2
	insertps xmmDst, real4 ptr [rbp - sizeof ymmword + sizeof real4 * i], INSERT_0_TO_1
endm

;xmmInt is intermediate register that will be modified. only xmmDst and xmmInt are used.
AbsDwordVec macro xmmDst, xmmInt
	vpsrad xmmInt, xmmDst, 31
	pxor xmmDst, xmmInt
	psubd xmmDst, xmmInt
endm

LoadLinePts macro finalPosLoad, initialPosLoad, diffPtLoad, pointDiffX, pointDiffY, lineRenderLHLbl, noRenderLbl, doneRenderLbl
	finalPosLoad xmm7, xmm4, xmm5;final single-precision floating point value pos
	cvttps2dq xmm7, xmm7;final dword pos
	initialPosLoad xmm8, xmm4, xmm5;initial pos
	diffPtLoad xmm9, pointDiffX, pointDiffY;difference

	RenderLine lineRenderLHLbl, noRenderLbl, doneRenderLbl
endm


RenderLine macro lineRenderLHLbl, noRenderLbl, doneRenderLbl
	
	vcmpss xmm11, xmm9, PosNanVec, EQU_VEC_CMP
	vcmpss xmm10, xmm9, NegNanVec, EQU_VEC_CMP
	por xmm11, xmm10
	movd eax, xmm11
	pextrd ecx, xmm11, 1
	and al, cl
	test al, 1
	jnz doneRenderLbl

	movaps xmm11, xmm9
	dpps xmm9, xmm9, VECTOR_2_DOT_MASK
	vcmpss xmm10, xmm9, real4 ptr [zero], GREATER_VEC_CMP
	rsqrtss xmm9, xmm9
	pand xmm9, xmm10
	vbroadcastss xmm9, xmm9
	mulps xmm9, xmm11
	
	cvttps2dq xmm10, xmm8
	movd ecx, xmm10
	movd edx, xmm7
	pextrd eax, xmm10, 1
	pextrd r8d, xmm7, 1

	pxor xmm11, xmm11
	vcmpps xmm11, xmm9, xmm11, LESS_EQU_VEC_CMP
	movd r9d, xmm11;r9b = xmm9.x == -ve
	pextrd r10d, xmm11, 1;r10b = xmm9.y == -ve
	
	pxor xmm11, xmm11
	vcmpps xmm11, xmm11, xmm9, LESS_EQU_VEC_CMP
	movd r11d, xmm11;r11b = xmm9.x == +ve
	pextrd r12d, xmm11, 1;r12b = xmm9.y == +ve


	cmp ecx, 0
	setle r13b
	and r13b, r9b
	
	cmp eax, 0
	setle r15b
	and r15b, r10b
	or r15b, r13b

	movzx r14d, word ptr [charBufferMaxVal + sizeof word * 2]
	cmp ecx, r14d
	setg r13b
	and r13b, r11b
	or r13b, r15b

	movzx r14d, word ptr [charBufferMaxVal + sizeof word * 3]
	cmp eax, r14d
	setg r15b
	and r15b, r12b
	or r15b, r13b

	
	sub ecx, edx
	mov r9d, ecx

	sar r9d, 31
	xor ecx, r9d
	sub ecx, r9d

	cmp ecx, 1
	setle cl
	sub eax, r8d
	mov r9d, eax

	sar r9d, 31
	xor eax, r9d
	sub eax, r9d

	cmp eax, 1
	setle al
	and al, cl
	or al, r15b
	test al, 1
	jnz doneRenderLbl

	mov eax, -1
lineRenderLHLbl:
		mov r13d, eax

		movaps xmm10, xmm8
		addps xmm8, xmm9
		movd eax, xmm10
		pextrd ecx, xmm10, 1
		movd r8d, xmm8
		pextrd r9d, xmm8, 1

		cmp eax, r8d
		sete r12b
		vcmpps xmm11, xmm9, zeroVec, NOT_EQU_VEC_CMP
		movd r10d, xmm11
		and r12b, r10b
		
		cmp ecx, r9d
		sete r10b
		pextrd r11d, xmm11, 1
		and r10b, r11b
		or r10b, r12b
		test r10b, 1
		jnz doneRenderLbl


		cvttps2dq xmm10, xmm8
		movd ecx, xmm10
		pextrd eax, xmm10, 1

		cmp ecx, 0
		setl dl

		cmp eax, 0
		setl r8b
		or dl, r8b

		movzx r9d, word ptr [charBufferMaxVal + sizeof word * 2]
		cmp ecx, r9d
		setg r8b
		or dl, r8b

		movzx r9d, word ptr [charBufferMaxVal + sizeof word * 3]
		cmp eax, r9d
		setg r8b
		or dl, r8b

		test dl, 1
		jnz noRenderLbl
		xor edx, edx
		mul ebx
		add eax, ecx
		add rax, charBuffer
		mov byte ptr [rax], 'o'
	noRenderLbl:

		psubd xmm10, xmm7
		AbsDwordVec xmm10, xmm11
		pmulld xmm10, xmm10
		movd eax, xmm10
		pextrd ecx, xmm10, 1
		add eax, ecx
		cmp eax, 2
		setl r14b
		cmp r13d, eax
		setnae r15b
		or r14b, r15b
		test r14b, 1
		jz lineRenderLHLbl

	doneRenderLbl:
endm

ProcessX macro curX, curY, intermediate, curPixel, const, loopHead, endFuncLabel
	vaddss intermediate, curY, curX
	addss intermediate, const
	comiss intermediate, real4 ptr [zero]
	ja endFuncLabel
	movups intermediate, xmmword ptr [ONE_AT_1]
	addps curPixel, intermediate
	jmp loopHead
endFuncLabel:

endm

;rcx: the first two triangle vertices (x1, y1, x2, y2), edx: the last triangle vertex ((dummy word), (dummy word), x3, y3). in other words, the registers store the vertex data BACKWARDS for caller simplification so that we can shift the vertices into the registers.
RasterizeTri proc
	push rbp
	mov rbp, rsp
	sub rsp, sizeof real4 * 9
	
	mov r10, rcx

	xor r11d, r11d
	mov r11w, r10w;		y2
	cvtsi2ss xmm1, r11d
	insertps xmm1, xmm1, INSERT_0_TO_1

	shr r10, NUM_BITS_IN_WORD

	xor r8d, r8d
	mov r8w, r10w;	x2
	cvtsi2ss xmm1, r8d

	shr r10, NUM_BITS_IN_WORD
	xor r9d, r9d
	mov r9w, r10w;	y1

	cvtsi2ss xmm0, r9d
	insertps xmm0, xmm0, INSERT_0_TO_1

	mov r12d, r11d
	cmp r11w, r9w
	;r11w: min(y1, y2)
	cmovg r11w, r9w
	;r12w: max(y1, y2)
	cmovl r12w, r9w
	shr r10, NUM_BITS_IN_WORD
	mov r9w, r10w;	x1
	cvtsi2ss xmm0, r9d

	mov r13d, r8d
	cmp r8w, r9w
	;r8w: min(x1, x2)
	cmovg r8w, r9w
	;r13w: max(x1, x2)
	cmovl r13w, r9w
	mov r9w, dx;	y3
	cvtsi2ss xmm2, r9d
	insertps xmm2, xmm2, INSERT_0_TO_1

	cmp r11w, r9w
	;r11w: min y
	cmovg r11w, r9w
	cvtsi2ss xmm6, r11d
	;min y @ rbp - sizeof real4 * 7
	movss real4 ptr [rbp - sizeof real4 * 7], xmm6
	cmp r12w, r9w
	;r12w: max y
	cmovl r12w, r9w
	;max y @ rbp - sizeof real4 * 5
	cvtsi2ss xmm6, r12d
	movss real4 ptr [rbp - sizeof real4 * 5], xmm6

	shr rdx, NUM_BITS_IN_WORD
	mov r9w, dx;	x3
	cvtsi2ss xmm2, r9d
	cmp r8w, r9w
	;r8w: min x
	cmovg r8w, r9w
	cvtsi2ss xmm6, r8d
	;min x @ rbp - sizeof real4 * 6
	movss real4 ptr [rbp - sizeof real4 * 6], xmm6
	cmp r13w, r9w
	;r13w: max x
	cmovl r13w, r9w
	;max x @ rbp - sizeof real4 * 4
	cvtsi2ss xmm6, r13d
	movss real4 ptr [rbp - sizeof real4 * 4], xmm6

	;xmm0: (x1, y1)
	;xmm1: (x2, y2)
	;xmm2: (x3, y3)

	;min x
	cvtsi2ss xmm9, r8d

	;min y
	cvtsi2ss xmm10, r11d
	
	;xmm10: (min y, min x)
	insertps xmm10, xmm9, INSERT_0_TO_1

	vsubps xmm3, xmm0, xmm1
	pshufd xmm3, xmm3, SHUFFLE_CROSS_X_Y
	mulss xmm3, negOne
	;xmm12: b.y - a.y vertex 0
	movss xmm12, xmm3
	insertps xmm6, xmm3, INSERT_1_TO_0
	;a.x - b.x vertex 0 @ rbp - sizeof real4
	movss real4 ptr [rbp - sizeof real4], xmm6

	;xmm3: p0.x(b.y - a.y) vertex 0
	mulss xmm3, real4 ptr [rbp - sizeof real4 * 6]
	insertps xmm9, xmm3, INSERT_1_TO_0
	;xmm9: p0.y(a.x - b.x) vertex 0
	mulss xmm9, xmm10

	vsubps xmm4, xmm1, xmm2
	pshufd xmm4, xmm4, SHUFFLE_CROSS_X_Y
	mulss xmm4, negOne
	;xmm13: b.y - a.y vertex 1
	movss xmm13, xmm4
	insertps xmm6, xmm4, INSERT_1_TO_0
	;a.x - b.x vertex 1 @ rbp - sizeof real4 * 2
	movss real4 ptr [rbp - sizeof real4 * 2], xmm6

	;xmm4: p0.x(b.y - a.y) vertex 1
	mulss xmm4, real4 ptr [rbp - sizeof real4 * 6]
	insertps xmm15, xmm4, INSERT_1_TO_0
	;xmm15: p0.y(a.x - b.x) vertex 1
	mulss xmm15, xmm10
	
	vsubps xmm5, xmm2, xmm0
	pshufd xmm5, xmm5, SHUFFLE_CROSS_X_Y
	mulss xmm5, negOne
	;xmm14: b.y - a.y vertex 2
	movss xmm14, xmm5
	insertps xmm6, xmm5, INSERT_1_TO_0
	;a.x - b.x vertex 2 @ rbp - sizeof real4 * 3
	movss real4 ptr [rbp - sizeof real4 * 3], xmm6

	;xmm5: p0.x(b.y - a.y) vertex 2
	mulss xmm5, real4 ptr [rbp - sizeof real4 * 6]
	insertps xmm11, xmm5, INSERT_1_TO_0
	;xmm11: p0.y(a.x - b.x) vertex 2
	mulss xmm11, xmm10
	
	pshufd xmm6, xmm0, SHUFFLE_CROSS_X_Y
	mulps xmm6, xmm1
	pshufd xmm7, xmm1, SHUFFLE_CROSS_X_Y
	mulps xmm7, xmm2
	pshufd xmm8, xmm2, SHUFFLE_CROSS_X_Y
	mulps xmm8, xmm0

	;xmm6: b.x * a.y - b.y * a.x vertex 0
	hsubps xmm6, xmm6
	
	;xmm7: b.x * a.y - b.y * a.x vertex 1
	hsubps xmm7, xmm7
	
	;xmm8: b.x * a.y - b.y * a.x vertex 2
	hsubps xmm8, xmm8
	

	;iff (p.x(b.y - a.y) + p.y(a.x - b.x) + b.x * a.y - b.y * a.x) > 0, the point is on the right side of the line.

	subss xmm10, one

rasterizeYHead:
	movss xmm1, real4 ptr [rbp - sizeof real4 * 6]

	movss xmm3, xmm1
	mulss xmm3, xmm12
	movss xmm4, xmm1
	mulss xmm4, xmm13
	movss xmm5, xmm1
	mulss xmm5, xmm14

	insertps xmm10, xmm1, INSERT_0_TO_1
	addss xmm10, one
	comiss xmm10, real4 ptr [rbp - sizeof real4 * 5]
	ja breakRasterizeY
	addss xmm9, real4 ptr [rbp - sizeof real4]
	addss xmm15, real4 ptr [rbp - sizeof real4 * 2]
	addss xmm11, real4 ptr [rbp - sizeof real4 * 3]
	rasterizeXHead:
		insertps xmm1, xmm10, INSERT_1_TO_0
		comiss xmm1, real4 ptr [rbp - sizeof real4 * 4]
		ja rasterizeYHead

		addss xmm3, xmm12
		addss xmm4, xmm13
		addss xmm5, xmm14

		ProcessX xmm3, xmm9, xmm1, xmm10, xmm6, rasterizeXHead, efLbl1

		ProcessX xmm4, xmm15, xmm1, xmm10, xmm7, rasterizeXHead, efLbl2

		ProcessX xmm5, xmm11, xmm1, xmm10, xmm8, rasterizeXHead, efLbl3
		
		cvtss2si rax, xmm10
		insertps xmm1, xmm10, INSERT_1_TO_0
		cvtss2si rcx, xmm1
		movzx rdx, word ptr [charbufferSize]
		mul rdx
		add rax, rcx
		add rax, qword ptr [charBuffer]
		mov byte ptr [rax], 'a'
		
		;xmm10: y, x
		movups xmm1, xmmword ptr [ONE_AT_1]
		addps xmm10, xmm1

		jmp rasterizeXHead
	jmp rasterizeYHead
breakRasterizeY:

	mov rsp, rbp
	pop rbp
	ret
RasterizeTri endp

;initial:
;0, 1, 2, 3, 4, 5, 6, 7

;desired:
;0, 4, 1, 5, 2, 6, 3, 7

;transformations:
;0, 4, 2, 3, 1, 5, 6, 7

;0, 4, 1, 3, 2, 5, 6, 7

;0, 4, 1, 6, 2, 5, 3, 7

;0, 4, 1, 5, 2, 6, 3, 7

;|  |  |  |  |  |  |  |
;|  |  |  |  |  |  |  |
;|  |  |  |  |  |  |  |
;\/ \/ \/ \/ \/ \/ \/ \/

;TODO: use more intermediate registers so that this can be done in less instructions
InterleaveVertices macro loc
	movss xmm3, real4 ptr [loc + sizeof real4 * 1]
	movss xmm11, real4 ptr [loc + sizeof real4 * 4]
	movss real4 ptr [loc + sizeof real4 * 4], xmm3
	movss real4 ptr [loc + sizeof real4 * 1], xmm11

	movss xmm3, real4 ptr [loc + sizeof real4 * 2]
	movss xmm11, real4 ptr [loc + sizeof real4 * 4]
	movss real4 ptr [loc + sizeof real4 * 4], xmm3
	movss real4 ptr [loc + sizeof real4 * 2], xmm11

	movss xmm3, real4 ptr [loc + sizeof real4 * 3]
	movss xmm11, real4 ptr [loc + sizeof real4 * 6]
	movss real4 ptr [loc + sizeof real4 * 6], xmm3
	movss real4 ptr [loc + sizeof real4 * 3], xmm11

	movss xmm3, real4 ptr [loc + sizeof real4 * 3]
	movss xmm11, real4 ptr [loc + sizeof real4 * 5]
	movss real4 ptr [loc + sizeof real4 * 5], xmm3
	movss real4 ptr [loc + sizeof real4 * 3], xmm11
endm

;;renders a cube at some location
;;ymm0: input x, ymm1: input y, ymm2: input z
RenderCubeLoc proc
	vmovups ymm4, ymmword ptr [cubeVerticesX]
	vmovups ymm5, ymmword ptr [cubeVerticesY]
	vmovups ymm6, ymmword ptr [cubeVerticesZ]

	vaddps ymm4, ymm4, ymm0
	vaddps ymm5, ymm5, ymm1
	vaddps ymm6, ymm6, ymm2
	
	push rbp
	mov rbp, rsp
	sub rsp, sizeof ymmword * 3 + sizeof ymmword * 3 + 20h + sizeof ymmword * 4 + sizeof xmmword
	vmovups ymmword ptr [rbp - sizeof ymmword * 1], ymm4;;;x
	vmovups ymmword ptr [rbp - sizeof ymmword * 2], ymm5;;;y
	vmovups ymmword ptr [rbp - sizeof ymmword * 3], ymm6;;;z

	rotated_points_loc equ rbp - sizeof ymmword * 3

	RotateVertices rotated_points_loc, 0
	RotateVertices rotated_points_loc, 1
	RotateVertices rotated_points_loc, 2
	RotateVertices rotated_points_loc, 3
	
	RotateVertices rotated_points_loc, 4
	RotateVertices rotated_points_loc, 5
	RotateVertices rotated_points_loc, 6
	RotateVertices rotated_points_loc, 7
	
	vmovups ymm4, ymmword ptr [rotated_points_loc - sizeof ymmword * 1]
	vmovups ymm5, ymmword ptr [rotated_points_loc - sizeof ymmword * 2]
	vmovups ymm6, ymmword ptr [rotated_points_loc - sizeof ymmword * 3]

	vmovups ymmword ptr [cubeVerticesX], ymm4
	vmovups ymmword ptr [cubeVerticesY], ymm5
	vmovups ymmword ptr [cubeVerticesZ], ymm6

	movzx eax, word ptr [charbufferSize]
	cvtsi2ss xmm0, eax
	movzx eax, word ptr [charbufferSize + 2]
	cvtsi2ss xmm1, eax
	vbroadcastss ymm2, xmm0
	vbroadcastss ymm3, xmm1

	vpxor ymm7, ymm7, ymm7
	vcmpps ymm7, ymm6, ymm7, GREATER_VEC_CMP; z more than zero ==> not visible to player

	;;vpxor ymm8, ymm8, ymmword ptr [NanVec]
	;;vpand ymm4, ymm4, ymm8
	;;vpand ymm5, ymm5, ymm8
	vpor ymm4, ymm4, ymm7
	vpor ymm5, ymm5, ymm7

	vdivps ymm4, ymm4, ymm6
	vdivps ymm5, ymm5, ymm6

	vmulps ymm4, ymm4, ymm2
	vmulps ymm5, ymm5, ymm3

	vmulps ymm0, ymm2, ymmword ptr [halves]
	vmulps ymm1, ymm3, ymmword ptr [halves]

	vaddps ymm4, ymm4, ymm0;final x floating-point vertices
	vaddps ymm5, ymm5, ymm1;final y floating-point vertices

	
	ymmwordLoc equ rbp - sizeof ymmword
	vertex2dLoc equ ymmwordLoc - sizeof ymmword

	xVertLoc equ rotated_points_loc - sizeof ymmword * 1
	yVertLoc equ rotated_points_loc - sizeof ymmword * 2

	vmovups ymmword ptr [xVertLoc], ymm4
	vmovups ymmword ptr [yVertLoc], ymm5

	InterleaveVertices xVertLoc
	InterleaveVertices yVertLoc

	movups xmm12, xmmword ptr [xVertLoc]
	movups xmm13, xmmword ptr [yVertLoc]
	
	vhsubps xmm14, xmm12, xmmword ptr [xVertLoc + sizeof xmmword]
	vhsubps xmm15, xmm13, xmmword ptr [yVertLoc + sizeof xmmword]
	
	vmovups ymmword ptr [ymmwordLoc], ymm4
	vhsubps xmm0, xmm4, xmmword ptr [ymmwordLoc + sizeof xmmword];floating point dif in x
	vmovups ymmword ptr [ymmwordLoc], ymm5
	vhsubps xmm1, xmm5, xmmword ptr [ymmwordLoc + sizeof xmmword];floating point dif in y

	last_real4_offset equ sizeof real4 * 7

	;;TODO: don't use vertex2dLoc; use xVertLoc & yVertLoc instead.
	vpslldq ymm3, ymm4, sizeof real4
	vmovups ymmword ptr [ymmwordLoc], ymm3
	vmovups ymmword ptr [vertex2dLoc], ymm4
	movss xmm11, real4 ptr [vertex2dLoc + sizeof real4 * 3]
	movss real4 ptr [ymmwordLoc], xmm11

	movss xmm11, real4 ptr [vertex2dLoc + sizeof real4 * 7]
	movss real4 ptr [ymmwordLoc + sizeof real4 * 4], xmm11
	vmovups ymm3, ymmword ptr [ymmwordLoc]

	vhsubps xmm3, xmm3, xmmword ptr [ymmwordLoc + sizeof xmmword];final second point dif in x

	vpslldq ymm6, ymm5, sizeof real4
	vmovups ymmword ptr [ymmwordLoc], ymm6
	vmovups ymmword ptr [vertex2dLoc], ymm5
	movss xmm11, real4 ptr [vertex2dLoc + sizeof real4 * 3]
	movss real4 ptr [ymmwordLoc], xmm11

	movss xmm11, real4 ptr [vertex2dLoc + sizeof real4 * 7]
	movss real4 ptr [ymmwordLoc + sizeof real4 * 4], xmm11
	vmovups ymm6, ymmword ptr [ymmwordLoc]

	vhsubps xmm6, xmm6, xmmword ptr [ymmwordLoc + sizeof xmmword];final second point dif in y

	push rbx
	push r12
	push r13
	push r14
	push r15
	cvtss2si ebx, xmm2

	LoadLinePts LoadFirstVerticalVertices, LoadVerticalVertices1, LoadFirstVerticalVertices, xmm0, xmm1, lh1, nr1, dontRender1
	LoadLinePts LoadVerticalVertices2, LoadVerticalVertices3, LoadVerticalVertices1, xmm0, xmm1, lh2, nr2, dontRender2
	
	LoadVerticalVerticesI xmm7, ymm4, ymm5, 4;final
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesI xmm8, ymm4, ymm5, 5;initial
	LoadVerticalVertices2 xmm9, xmm0, xmm1;difference
	RenderLine lh4, nr4, dr4
	
	LoadVerticalVerticesI xmm7, ymm4, ymm5, 6;final
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesI xmm8, ymm4, ymm5, 7;initial
	LoadVerticalVertices3 xmm9, xmm0, xmm1;difference
	RenderLine lh5, nr5, dr5
	
	LoadVerticalVertices3 xmm7, xmm4, xmm5;initial
	cvttps2dq xmm7, xmm7
	LoadFirstVerticalVertices xmm8, xmm4, xmm5;final
	LoadFirstVerticalVertices xmm9, xmm3, xmm6;difference
	RenderLine lh6, nr6, dontRender6
	
	LoadVerticalVertices1 xmm7, xmm4, xmm5;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVertices2 xmm8, xmm4, xmm5;final
	LoadVerticalVertices1 xmm9, xmm3, xmm6;difference
	RenderLine lh7, nr7, dontRender7
	
	LoadVerticalVerticesI xmm7, ymm4, ymm5, 7;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesI xmm8, ymm4, ymm5, 4;final
	LoadVerticalVertices2 xmm9, xmm3, xmm6;difference
	RenderLine lh8, nr8, dontRender8
	
	LoadVerticalVerticesI xmm7, ymm4, ymm5, 5;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesI xmm8, ymm4, ymm5, 6;final
	LoadVerticalVertices3 xmm9, xmm3, xmm6;difference
	RenderLine lh9, nr9, dontRender9
	
	LoadFirstVerticalVertices xmm7, xmm4, xmm5;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesI xmm8, ymm4, ymm5, 4;final
	LoadFirstVerticalVertices xmm9, xmm14, xmm15;difference
	RenderLine lh10, nr10, dontRender10
	
	LoadVerticalVertices1 xmm7, xmm4, xmm5;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesI xmm8, ymm4, ymm5, 5;final
	LoadVerticalVertices1 xmm9, xmm14, xmm15;difference
	RenderLine lh11, nr11, dontRender11
	
	LoadVerticalVertices2 xmm7, xmm4, xmm5;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesI xmm8, ymm4, ymm5, 6;final
	LoadVerticalVertices2 xmm9, xmm14, xmm15;difference
	RenderLine lh12, nr12, dontRender12
	
	LoadVerticalVertices3 xmm7, xmm4, xmm5;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesI xmm8, ymm4, ymm5, 7;final
	LoadVerticalVertices3 xmm9, xmm14, xmm15;difference
	RenderLine lh13, nr13, dontRender13
	
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx

	;;;uses: ymm2, ymm4, ymm5
	;;; | | | | |
	;;; | | | | |
	;;; \/\/\/\/\/

	vcvttps2dq ymm4, ymm4;final doubleword x vertices
	vcvttps2dq ymm5, ymm5;final doubleword y vertices
	vmulps ymm5, ymm5, ymm2
	movzx eax, word ptr [charBufferMaxVal + sizeof word * 2]
	movd xmm3, eax
	vpbroadcastd ymm3, xmm3
	vpcmpgtd ymm7, ymm4, ymm3
	vpxor ymm3, ymm3, ymm3
	vpcmpgtd ymm0, ymm3, ymm4
	vpor ymm7, ymm7, ymm0

	vpcmpgtd ymm0, ymm3, ymm5
	vpor ymm7, ymm7, ymm0
	movd xmm1, maxCharValue
	vpbroadcastd ymm1, xmm1
	vpcmpgtd ymm1, ymm5, ymm1
	vpor ymm7, ymm7, ymm1
	vpor ymm5, ymm5, ymm7
	vpor ymm4, ymm4, ymm7
	vpaddd ymm0, ymm4, ymm5;screen pos

	;;;vmovups ymm4, ymmword ptr [temp]
	;;;vmovups ymm5, ymmword ptr [temp2]
	;;;;x1, x1, x3, x3, x5, x5, x7, x7
	;;;vmovsldup ymm6, ymm4;even
	;;;;x2, x2, x4, x4, x6, x6, x8, x8
	;;;vmovshdup ymm7, ymm4;odd
	;;;vaddps ymm6, ymm6, ymm7

	vmovups ymmword ptr [rsp], ymm0
	mov ecx, sizeof ymmword / sizeof dword
renderCharHead:
		mov eax, dword ptr [rsp + rcx * sizeof dword - sizeof dword]
		cmp eax, 0
		jl loopEnd
		add rax, charBuffer
		mov byte ptr [rax], 'O'
loopEnd:
	loop renderCharHead
	mov rsp, rbp
	pop rbp

	ret
RenderCubeLoc endp
endif