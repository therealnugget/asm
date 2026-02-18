ifndef renderer
renderer equ 1
.data
SHUFFLE_ODD equ 136
SHUFFLE_EVEN equ 221
MORE_THAN_VEC_CMP_PS equ 5

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

;default cube veritces
cubeVerticesX real4 0.5,  0.5,  0.5,  0.5, -0.5, -0.5, -0.5, -0.5
cubeVerticesY real4 0.5,  0.5, -0.5, -0.5,  0.5,  0.5, -0.5, -0.5
cubeVerticesZ real4 0.5, -0.5, -0.5,  0.5,  0.5, -0.5, -0.5,  0.5
SingleDefVec halves, 0.5
SingledefVec zeroVec, 0.0
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
	vcmpps ymm7, ymm6, ymm7, MORE_THAN_VEC_CMP_PS; z more than zero ==> not visible to player
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

	vaddps ymm4, ymm4, ymm0
	vaddps ymm5, ymm5, ymm1

	
	xVertexLoc equ rbp - sizeof ymmword
	yVertexLoc equ rbp - sizeof ymmword * 2
	xFinalLoc equ rbp - sizeof ymmword * 2 - sizeof ymmword
	yFinalLoc equ rbp - sizeof ymmword * 2 - sizeof ymmword * 2

	vmovups ymmword ptr [xVertexLoc], ymm4
	vmovups ymmword ptr [yVertexLoc], ymm5
	movups xmm0, xmmword ptr [xVertexLoc]
	vhsubps xmm0, xmm0, xmmword ptr [xVertexLoc + sizeof xmmword]
	movups xmm1, xmmword ptr [yVertexLoc]
	vhsubps xmm1, xmm1, xmmword ptr [yVertexLoc + sizeof xmmword]

lineRenderLoopHead:
		cvttps2dq xmm0, xmm0
		cvttps2dq xmm1, xmm1


	movss xmm2, xmm0
	insertps xmm2, xmm1, INSERT_0_TO_1

	vcvttps2dq ymm4, ymm4;final doubleword x vertices
	vcvttps2dq ymm5, ymm5;y vertices
	vmulps ymm5, ymm5, ymm2;final doubleword y vertices
	xor eax, eax
	mov ax, word ptr [charBufferMaxVal + 4]
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