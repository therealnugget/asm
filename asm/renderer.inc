ifndef renderer
renderer equ 1
.data
SHUFFLE_ODD equ 136
SHUFFLE_EVEN equ 221
MORE_THAN_VEC_CMP_PS equ 5
;default cube veritces
cubeVerticesX real4 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5
cubeVerticesY real4 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, -0.5
cubeVerticesZ real4 0.5, -0.5,	-0.5, 0.5, 0.5, -0.5, -0.5, 0.5
SingleDefVec half, halves, 0.5
SingleDefVec oneS, oneVec, -1.0
NanVec dd -1, -1, -1, -1, -1, -1, -1, -1
;;;temp dword 0, 1, 2, 3, 4, 5, 6, 7
;;;temp2 dword 8, 9, 10, 11, 12, 13, 14, 15
.code

;;renders a cube at some location with some scale
;;ymm0: scale, ymm1: positionX, ymm2: positionY, ymm3: positionZ
RenderCubeLocScale proc scale: ymmword
	vmovups ymm4, ymmword ptr [cubeVerticesX]
	vmovups ymm5, ymmword ptr [cubeVerticesY]
	vmovups ymm6, ymmword ptr [cubeVerticesZ]
	vmulps ymm4, ymm4, ymm0
	vmulps ymm5, ymm5, ymm0
	vmulps ymm6, ymm6, ymm0
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
	;;vpxor ymm8, ymm8, NanVec
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

	sub rsp, sizeof ymmword
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
	add rsp, sizeof ymmword

	ret
RenderCubeLocScale endp
endif