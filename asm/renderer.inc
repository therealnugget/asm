ifndef renderer
renderer equ 1
.data
SHUFFLE_ODD equ 136
SHUFFLE_EVEN equ 221
FLT_MIN_I equ -26172378

EXTRACT_LOW_WORD equ 0

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

;index buffer
;1

;default cube veritces
cubeVerticesX real4 0.5,  0.5,  0.5,  0.5, -0.5, -0.5, -0.5, -0.5
cubeVerticesY real4 0.5,  0.5, -0.5, -0.5,  0.5,  0.5, -0.5, -0.5
cubeVerticesZ real4 0.5, -0.5, -0.5,  0.5,  0.5, -0.5, -0.5,  0.5
SingleDefVec halves, 0.5
SingledefVec zeroVec, 0.0
float_min real4 -26172378.0
NanVec dd -1, -1, -1, -1, -1, -1, -1, -1
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

RotateVertices macro STORE_LOCATION, VERTEX_OFFSET

	MAX_RET_LOC equ STORE_LOCATION - sizeof ymmword * 3
	
	mov al, rotationX
	movups xmm0, xmmword ptr [XAxis]
	call QuatAngleAxis
	movups xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4 - sizeof xmmword], xmm0

	mov al, rotationY
	movups xmm0, xmmword ptr [YAxis]
	call QuatAngleAxis
	movups xmm1, xmmword ptr [MAX_RET_LOC - sizeof ymmword * 4 - sizeof xmmword]
	call QuatMult

	ConvertHorizonPtToVert MAX_RET_LOC, VERTEX_OFFSET

	call QuatMultVec
	vmovups ymmword ptr [rsp], ymm0
	mov eax, dword ptr [rsp + sizeof real4 * 0]
	mov dword ptr [STORE_LOCATION + VERTEX_OFFSET - sizeof ymmword * 1], eax
	mov eax, dword ptr [rsp + sizeof real4 * 1]
	mov dword ptr [STORE_LOCATION + VERTEX_OFFSET - sizeof ymmword * 2], eax
	mov eax, dword ptr [rsp + sizeof real4 * 2]
	mov dword ptr [STORE_LOCATION + VERTEX_OFFSET - sizeof ymmword * 3], eax
endm

RestoreDefaultPoints macro
	vmovups ymm4, ymmword ptr [cubeVerticesX]
	vmovups ymm5, ymmword ptr [cubeVerticesY]
	vmovups ymm6, ymmword ptr [cubeVerticesZ]
endm

RestoreVertices macro
	vmovups ymm4, ymmword ptr [rbp - sizeof ymmword * 5]
	vmovups ymm5, ymmword ptr [rbp - sizeof ymmword * 6]
	vmovups ymm6, ymmword ptr [rbp - sizeof ymmword * 7]
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
;;;;ANOTHER ONE
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

LoadLinePts macro finalPosLoad, initialPosLoad, diffPtLoad, pointDiffX, pointDiffY, lineRenderLHLbl, noRenderLbl, doneRenderLbl, temp
	finalPosLoad xmm7, xmm4, xmm5;final single-precision floating point value pos
	cvttps2dq xmm7, xmm7;final dword pos
	initialPosLoad xmm8, xmm4, xmm5;initial pos
	diffPtLoad xmm9, pointDiffX, pointDiffY;difference

	RenderLine lineRenderLHLbl, noRenderLbl, doneRenderLbl, temp
endm


RenderLine macro lineRenderLHLbl, noRenderLbl, doneRenderLbl, temp
	
	movaps xmm11, xmm9
	dpps xmm9, xmm9, VECTOR_2_DOT_MASK
	pand xmm9, xmmword ptr [ABS_VALUE_MASK_F]
	rsqrtss xmm9, xmm9
	vbroadcastss xmm9, xmm9
	mulps xmm9, xmm11
	
	cvttps2dq xmm10, xmm8
	movd ecx, xmm10
	movd edx, xmm7
	pextrd eax, xmm10, 1
	pextrd r8d, xmm7, 1

	;pxor xmm11, xmm11
	;vcmpps xmm11, xmm9, xmm11, LESS_EQU_VEC_CMP
	;movd r9d, xmm11;r9b = xmm11.x == -ve
	;pextrd r10d, xmm11, 1;r10b = xmm11.y == -ve
	;
	;pxor xmm11, xmm11
	;vcmpps xmm11, xmm9, xmm11, GREATER_VEC_CMP
	;movd r11d, xmm11;r11b = xmm11.x == +ve
	;pextrd r12d, xmm11, 1;r12b = xmm11.y == +ve


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

	
	cmp ecx, edx
	sete cl
	cmp eax, r8d
	sete al
	and al, cl
	test al, 1
	jnz doneRenderLbl

lineRenderLHLbl:
		movaps xmm10, xmm8
		addps xmm8, xmm9
		movd eax, xmm10
		pextrd ecx, xmm10, 1
		movd r8d, xmm8
		pextrd r9d, xmm8, 1

		cmp eax, r8d
		sete r9b
		vcmpps xmm11, xmm9, zeroVec, NOT_EQU_VEC_CMP
		movd ecx, xmm11
		and r9b, cl
		
		cmp ecx, r9d
		sete r10b
		pextrd r11d, xmm11, 1
		and r10b, r11b
		or r10b, r9b
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
		movd eax, xmm10
		pextrd ecx, xmm10, 1
		add eax, ecx
		cmp eax, 1
		jg lineRenderLHLbl

	doneRenderLbl:
endm

;;renders a cube at some location with some scale
;;ymm0: scale, ymm1: positionX, ymm2: positionY, ymm3: positionZ
;scale: ymmword
RenderCubeLocScale proc
	vmovups ymm4, ymmword ptr [cubeVerticesX]
	vmovups ymm5, ymmword ptr [cubeVerticesY]
	vmovups ymm6, ymmword ptr [cubeVerticesZ]

	vmulps ymm4, ymm4, ymm0
	vmulps ymm5, ymm5, ymm0
	vmulps ymm6, ymm6, ymm0
	
	push rbp
	mov rbp, rsp
	sub rsp, sizeof ymmword * 7 + sizeof ymmword * 3 + 20h + sizeof ymmword * 4 + sizeof xmmword
	vmovups ymmword ptr [rbp - sizeof ymmword], ymm0
	vmovups ymmword ptr [rbp - sizeof ymmword * 2], ymm1
	vmovups ymmword ptr [rbp - sizeof ymmword * 3], ymm2
	vmovups ymmword ptr [rbp - sizeof ymmword * 4], ymm3
	vmovups ymmword ptr [rbp - sizeof ymmword * 5], ymm4
	vmovups ymmword ptr [rbp - sizeof ymmword * 6], ymm5
	vmovups ymmword ptr [rbp - sizeof ymmword * 7], ymm6

	rotated_points_loc equ rbp - sizeof ymmword * 7

	RotateVertices rotated_points_loc, sizeof real4 * 0
	RotateVertices rotated_points_loc, sizeof real4 * 1
	RotateVertices rotated_points_loc, sizeof real4 * 2
	RotateVertices rotated_points_loc, sizeof real4 * 3

	RotateVertices rotated_points_loc, sizeof real4 * 4
	RotateVertices rotated_points_loc, sizeof real4 * 5
	RotateVertices rotated_points_loc, sizeof real4 * 6
	RotateVertices rotated_points_loc, sizeof real4 * 7
	
	RestoreVertices

	vmovups ymm4, ymmword ptr [rotated_points_loc - sizeof ymmword * 1]
	vmovups ymm5, ymmword ptr [rotated_points_loc - sizeof ymmword * 2]
	vmovups ymm6, ymmword ptr [rotated_points_loc - sizeof ymmword * 3]


	vmovups ymm0, ymmword ptr [rbp - sizeof ymmword]
	vmovups ymm1, ymmword ptr [rbp - sizeof ymmword * 2]
	vmovups ymm2, ymmword ptr [rbp - sizeof ymmword * 3]
	vmovups ymm3, ymmword ptr [rbp - sizeof ymmword * 4]

	vaddps ymm4, ymm4, ymm1
	vaddps ymm5, ymm5, ymm2
	vaddps ymm6, ymm6, ymm3

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

	
	;;TODO: try to change the movups from mem to xmm and visa-versa to movaps here and test if it works
	vmovups ymmword ptr [ymmwordLoc], ymm4
	vhsubps xmm0, xmm4, xmmword ptr [ymmwordLoc + sizeof xmmword];floating point dif in x
	vmovups ymmword ptr [ymmwordLoc], ymm5
	vhsubps xmm1, xmm5, xmmword ptr [ymmwordLoc + sizeof xmmword];floating point dif in y

	last_real4_offset equ sizeof real4 * 7

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

	LoadLinePts LoadFirstVerticalVertices, LoadVerticalVertices1, LoadFirstVerticalVertices, xmm0, xmm1, lh1, nr1, dontRender1, t2
	LoadLinePts LoadVerticalVertices2, LoadVerticalVertices3, LoadVerticalVertices1, xmm0, xmm1, lh2, nr2, dontRender2, t3
	
	LoadVerticalVerticesi xmm7, ymm4, ymm5, 4;final
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesi xmm8, ymm4, ymm5, 5;initial
	LoadVerticalVertices2 xmm9, xmm0, xmm1;difference
	RenderLine lh4, nr4, dr4, t4
	
	LoadVerticalVerticesi xmm7, ymm4, ymm5, 6;final
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesi xmm8, ymm4, ymm5, 7;initial
	LoadVerticalVertices3 xmm9, xmm0, xmm1;difference
	RenderLine lh5, nr5, dr5, t5
	
	LoadVerticalVertices3 xmm7, xmm4, xmm5;initial
	cvttps2dq xmm7, xmm7
	LoadFirstVerticalVertices xmm8, xmm4, xmm5;final
	LoadFirstVerticalVertices xmm9, xmm3, xmm6;difference
	RenderLine lh6, nr6, dontRender6, t6
	
	LoadVerticalVertices1 xmm7, xmm4, xmm5;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVertices2 xmm8, xmm4, xmm5;final
	LoadVerticalVertices1 xmm9, xmm3, xmm6;difference
	RenderLine lh7, nr7, dontRender7, t7
	
	LoadVerticalVerticesi xmm7, ymm4, ymm5, 7;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesi xmm8, ymm4, ymm5, 4;final
	LoadVerticalVertices2 xmm9, xmm3, xmm6;difference
	RenderLine lh8, nr8, dontRender8, t8
	
	LoadVerticalVerticesi xmm7, ymm4, ymm5, 5;initial
	cvttps2dq xmm7, xmm7
	LoadVerticalVerticesi xmm8, ymm4, ymm5, 6;final
	LoadVerticalVertices3 xmm9, xmm3, xmm6;difference
	RenderLine lh9, nr9, dontRender9, t9
	
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
	;it unsettles me that this works. it should be that you multiply by the packed singles *before* converting.
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
RenderCubeLocScale endp
endif