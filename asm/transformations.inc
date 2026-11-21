ifndef transformationsInc
.data
transformationsInc equ 1

.code
ResetPosOrigin macro ymmPosition, ymmIntemediate1, xmmIntermediate1, intGPR, centreOffset
	movss xmmIntermediate1, real4 ptr [intGPR + sizeof real4 * centreOffset]
	vbroadcastss ymmIntemediate1, xmmIntermediate1
	vsubps ymmPosition, ymmPosition, ymmIntemediate1
endm

;;moves a cube to a new location
;;xmm0: position (x, y, z, dummy) (dummy is highest part), r9: data of cube (Cube struct)
SetCubePosition proc
	push rbp
	mov rbp, rsp
	sub rsp, sizeof xmmword
	movups xmmword ptr [rsp], xmm6

	lea rax, (Cube ptr [r9]).centre
	vmovups ymm3, (Cube ptr [r9]).xVert
	vmovups ymm4, (Cube ptr [r9]).yVert
	vmovups ymm5, (Cube ptr [r9]).zVert

	ResetPosOrigin ymm3, ymm6, xmm6, rax, 0
	ResetPosOrigin ymm4, ymm6, xmm6, rax, 1
	ResetPosOrigin ymm5, ymm6, xmm6, rax, 2

	movups xmmword ptr [rax], xmm0

	insertps xmm1, xmm0, INSERT_1_TO_0
	insertps xmm2, xmm0, INSERT_2_TO_0
	vbroadcastss ymm0, xmm0
	vbroadcastss ymm1, xmm1
	vbroadcastss ymm2, xmm2
	vaddps ymm3, ymm3, ymm0
	vaddps ymm4, ymm4, ymm1
	vaddps ymm5, ymm5, ymm2

	vmovups (Cube ptr [r9]).xVert, ymm3
	vmovups (Cube ptr [r9]).yVert, ymm4
	vmovups (Cube ptr [r9]).zVert, ymm5

	movups xmm6, xmmword ptr [rsp]
	mov rsp, rbp
	pop rbp
	ret
SetCubePosition endp
endif