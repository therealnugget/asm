ifndef quaternionInc
.data
quaternionInc equ 1
QUATERNION_DOT_MASK equ 0f1h
VECTOR_DOT_MASK equ 071h
;;i.e. highest to lowest
W_TO_X_MASK equ 3h
SHUFFLE_CROSS_X_0 equ 201
SHUFFLE_CROSS_Y_0 equ 210
SHUFFLE_CROSS_X_1 equ 210
SHUFFLE_CROSS_Y_1 equ 210
INSERT_W_TO_X equ 0c0h
INSERT_W_TO_W equ 0f0h
INSERT_X_TO_W equ 30h
NEQU_VEC_CMP_SS equ 3h
EQU_VEC_CMP_SS equ 0h
LESS_EQU_VEC_CMP_SS equ 2h
GREATER_VEC_CMP_SS equ 6h
PI_ON_4 equ 63

SingleDefVec negOneVec, -1.0
SingleDefVec twoVec, 2.0
SingleDefVec negTwoVec, -2.0
twoFiddyfahv real4 255.0
one real4 1.0
zero real4 0.0
negTwo real4 -2.0
two real4 2.0
negOne real4 -1.0
acos_iteration_0 real4 -0.0187293
acos_iteration_1 real4 0.0742610
acos_iteration_2 real4 -0.2121144
acos_iteration_3 real4 1.5707288
forward_vector3 real4 0.0, 0.0, 1.0
pi real4 3.14159265358979
align 16
ABS_VALUE_MASK dd 7FFFFFFFh, 0, 0, 0
QUATERNION_W_EXTRACTION dd 0, 0, 0, -1
QUATERNION_XYZ_EXTRACTION dd -1, -1, -1, 0

sinLookupTable db 0, 6, 13, 19, 25, 31, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92, 98
db 104, 109, 115, 121, 126, 132, 137, 142, 147, 152, 157, 162, 167, 172, 176, 181, 185, 190, 194, 198, 202, 205
db 209, 213, 216, 219, 222, 225, 228, 231, 234, 236, 238, 241, 243, 244, 246, 248
db 249, 250, 251, 252, 253, 254, 254, 255, 255, 255, 255, 255, 254, 254, 253, 252, 251, 250, 248, 247, 245, 243
db 242, 239, 237, 235, 232, 230, 227, 224, 221, 218, 214, 211, 207, 203, 200, 196
db 192, 187, 183, 179, 174, 169, 165, 160, 155, 150, 145, 140, 134, 129, 123, 118, 112, 107, 101, 95, 89, 83, 77
db 71, 65, 59, 53, 47, 41, 34, 28, 22, 16, 9, 3, -3, -9, -16, -22, -28, -34, -41, -47, -53, -59, -65, -71, -77
db -83, -89, -95, -101, -107, -112, -118, -123, -129, -134, -140, -145, -150, -155
db -160, -165, -169, -174, -179, -183, -187, -192, -196, -200, -203, -207, -211, -214
db -218, -221, -224, -227, -230, -232, -235, -237, -239, -242, -243, -245, -247, -248, -250, -251, -252, -253
db -254, -254, -255, -255, -255, -255, -255, -254, -254, -253, -252, -251, -250, -249, -248, -246, -244, -243
db -241, -238, -236, -234, -231, -228, -225, -222, -219, -216, -213, -209, -205
db -202, -198, -194, -190, -185, -181, -176, -172, -167, -162, -157, -152, -147, -142, -137, -132, -126, -121
db -115, -109, -104, -98, -92, -86, -80, -74, -68, -62, -56, -50, -44, -38, -31, -25
db -19, -13, -6

Quaternion struct
	real4 x 0.0
	real4 y 0.0
	real4 z 0.0
	real4 w 1.0
Quaternion ends

.code
;TODO: test if this function works
;xmm0: angle in radians. returns arccos in xmm0
Acos proc
	movaps xmm2, xmm0
	pand xmm0, xmmword ptr [ABS_VALUE_MASK]
	cmpss xmm2, xmm0, NEQU_VEC_CMP_SS
	movss xmm1, acos_iteration_0
	mulss xmm1, xmm0
	addss xmm1, acos_iteration_1
	mulss xmm1, xmm0
	addss xmm1, acos_iteration_2
	mulss xmm1, xmm0
	addss xmm1, acos_iteration_3
	mulss xmm0, negOne
	addss xmm0, one
	sqrtss xmm0, xmm0
	mulss xmm1, xmm0;ret *= sqrt(1.0 - x)
	vmulss xmm3, xmm1, negTwo
	addss xmm3, pi
	pand xmm3, xmm2
	addss xmm1, xmm3
	movss xmm0, xmm1
	ret
Acos endp

;xmm0: vector3 a, xmm1: vector3 b. returns the cross product in xmm0. for all intents and purposes consider xmm0's w value to be an undefined arbitrary value
VecCross proc
	movaps xmm2, xmm0
	pshufd xmm0, xmm0, SHUFFLE_CROSS_X_0
	pshufd xmm1, xmm1, SHUFFLE_CROSS_Y_0
	mulps xmm0, xmm1
	pshufd xmm2, xmm2, SHUFFLE_CROSS_X_1
	pshufd xmm1, xmm1, SHUFFLE_CROSS_Y_1
	mulps xmm1, xmm2
	subps xmm0, xmm1
	ret
VecCross endp

;xmmReg0: quaternion a, xmmReg1: quaternion b. high order bits of xmmReg0 are cleared, bits [255:128] of ymmReg0 are not affected
QuatDot macro xmmReg0, xmmReg1
	dpps xmmReg0, xmmReg1, QUATERNION_DOT_MASK
endm

;xmmReg0: vector a, xmmReg1: vector b. high order bits of xmmReg0 are cleared, bits [255:128] of ymmReg0 are not affected
VectorDot macro xmmReg0, xmmReg1
	dpps xmmReg0, xmmReg1, VECTOR_DOT_MASK
endm

;xmmReg: quaternion to test the squared length of. high order bits of xmmReg are cleared, bits [255:128] of ymmReg are not affected
QuatLengthSqr macro xmmReg
	dpps xmmReg, xmmReg, QUATERNION_DOT_MASK
endm

;xmm0: quaternion to test the length of
;returns length of the quaternion xmm0 as a scalar in xmm0 and leaves the high-order singles unmodified.
QuatLength proc
	dpps xmm0, xmm0, QUATERNION_DOT_MASK
	sqrtss xmm0, xmm0
	ret
QuatLength endp

;normalizes quaternion in xmm0
QuatNormalize proc
	dpps xmm0, xmm0, QUATERNION_DOT_MASK
	vcmpss xmm2, xmm0, zero, GREATER_VEC_CMP_SS
	vbroadcastss xmm2, xmm2
	rsqrtss xmm1, xmm0
	vbroadcastss xmm1, xmm1
	mulps xmm0, xmm1
	pand xmm0, xmm2
	ret
QuatNormalize endp

;xmm0: quaternion a, xmm1: quaternion b. returns the quaternion multiplication.
QuatMult proc
	vpshufd xmm2, xmm0, W_TO_X_MASK
	vbroadcastss xmm2, xmm2
	mulps xmm2, xmm1

	pshufd xmm3, xmm1, W_TO_X_MASK
	vbroadcastss xmm3, xmm3
	mulps xmm3, xmm0
	addps xmm3, xmm2
	
	movaps xmm4, xmm0
	movaps xmm5, xmm1

	;;i copied out this code so that i didn't have to call VecCross, that way i don't have to push and pop xmm3. the alternatives being to push and pop (very slow), or to assume that the function leaves it unaffected (incredibly error-prone and bad convention).
	movaps xmm2, xmm0
	pshufd xmm0, xmm0, SHUFFLE_CROSS_X_0
	pshufd xmm1, xmm1, SHUFFLE_CROSS_Y_0
	mulps xmm0, xmm1
	pshufd xmm2, xmm2, SHUFFLE_CROSS_X_1
	pshufd xmm1, xmm1, SHUFFLE_CROSS_Y_1
	mulps xmm1, xmm2
	subps xmm0, xmm1
	addps xmm0, xmm3;a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz)
	
	pshufd xmm3, xmm4, W_TO_X_MASK;w of a in xmm3
	pshufd xmm1, xmm5, W_TO_X_MASK;w of b in xmm1
	;all bits higher than least significant single-precision floating point value are zeroed
	dpps xmm4, xmm5, VECTOR_DOT_MASK
	mulss xmm1, xmm3
	subss xmm1, xmm4
	insertps xmm0, xmm1, INSERT_W_TO_W

	ret
QuatMult endp

;xmm0: quaternion, xmm1: scalar single-precision floating point value
QuatMultS proc
	vbroadcastss xmm1, xmm1
	mulps xmm0, xmm1
QuatMultS endp

;xmm0: quaternion, xmm1: scalar single-precision floating point value
QuatDivS proc
	vbroadcastss xmm1, xmm1
	divps xmm0, xmm1
QuatDivS endp

;xmm0: quaternion to conjugate. returns conjugate in xmm0
QuatConj proc
	vpand xmm1, xmm0, xmmword ptr [QUATERNION_XYZ_EXTRACTION]
	mulps xmm1, negTwoVec
	addps xmm0, xmm1

	;;alternative:
	;;;vpand xmm1, xmm0, QUATERNION_XYZ_EXTRACTION
	;;;vpand xmm0, xmm0, QUATERNION_W_EXTRACTION
	;;;mulps xmm1, negOneVec
	;;;vpor xmm0, xmm0, xmm1
	ret
QuatConj endp

;xmm0: quaternion to invert
QuatInv proc
	vpand xmm1, xmm0, xmmword ptr [QUATERNION_XYZ_EXTRACTION]
	mulps xmm1, negTwoVec
	addps xmm1, xmm0

	QuatLengthSqr xmm0
	divps xmm1, xmm0
	movaps xmm0, xmm1
	ret
QuatInv endp

;xmm0: quaternion, xmm1: vector3. returns a vector in xmm0 which represents the quaternion arg multiplied by the vector arg. bits [127:96] is an undefined arbitrary value
QuatMultVec proc
	movaps xmm5, xmm0
	movaps xmm4, xmm1
	pshufd xmm3, xmm0, W_TO_X_MASK
	vbroadcastss xmm3, xmm3
	
	movaps xmm2, xmm0
	pshufd xmm0, xmm0, SHUFFLE_CROSS_X_0
	pshufd xmm1, xmm1, SHUFFLE_CROSS_Y_0
	mulps xmm0, xmm1
	pshufd xmm2, xmm2, SHUFFLE_CROSS_X_1
	pshufd xmm1, xmm1, SHUFFLE_CROSS_Y_1
	mulps xmm1, xmm2
	subps xmm0, xmm1
	mulps xmm0, twoVec;xmm0 = 2 * (q.xyz X V)

	mulps xmm3, xmm0;xmm3 = q.w * 2 * (q.xyz X v)
	addps xmm3, xmm4;xmm3 = xmm3 + v
	
	movaps xmm1, xmm0;xmm0 is still: 2 * (q.xyz X V)
	movaps xmm0, xmm5

	movaps xmm2, xmm0
	pshufd xmm0, xmm0, SHUFFLE_CROSS_X_0
	pshufd xmm1, xmm1, SHUFFLE_CROSS_Y_0
	mulps xmm0, xmm1
	pshufd xmm2, xmm2, SHUFFLE_CROSS_X_1
	pshufd xmm1, xmm1, SHUFFLE_CROSS_Y_1
	mulps xmm1, xmm2
	subps xmm0, xmm1

	addps xmm0, xmm3
	ret
QuatMultVec endp

;xmm0: quaternion to get angle of. returns angle in xmm0 in radians
QuatAngle proc
	pshufd xmm0, xmm0, W_TO_X_MASK
	sub rsp, 20h
	call Acos
	add rsp, 20h
	mulss xmm0, two
	ret
QuatAngle endp

;xmm0: quaternion to get axis of. returns axis vector3 in xmm0
QuatAxis proc
	pshufd xmm1, xmm0, W_TO_X_MASK
	mulss xmm1, xmm1
	mulss xmm1, negOne
	addss xmm1, one
	rsqrtss xmm2, xmm1
	cmpss xmm1, zero, GREATER_VEC_CMP_SS
	vbroadcastss xmm1, xmm1
	vbroadcastss xmm2, xmm2
	mulps xmm0, xmm2;orig q.xyz / sqrt(q.w)
	pand xmm0, xmm1
	ret
QuatAxis endp

;xmm0: the vector3 to normalize. returns the normalized vector in xmm0
VecNormalize proc
	movaps xmm1, xmm0
	VectorDot xmm0, xmm0
	vcmpss xmm3, xmm0, zero, GREATER_VEC_CMP_SS
	vbroadcastss xmm3, xmm3
	rsqrtss xmm0, xmm0
	pand xmm0, xmm3
	vbroadcastss xmm0, xmm0
	mulps xmm0, xmm1
	ret
VecNormalize endp

;al: a value in the range [0:255] which represents angle, xmm0: vector3. returns the quaternion Rotated around the axis in xmm0
QuatAngleAxis proc
	push rbp
	mov rbp, rsp
	sub rsp, 20h + sizeof word + sizeof xmmword * 2 + 6

	call VecNormalize
	movups xmmword ptr [rbp - sizeof word - sizeof xmmword], xmm0

	movzx ax, al
	mov cl, 2
	div cl
	mov word ptr [rbp - sizeof word], ax
	call Sin
	movss xmm2, xmm0
	vbroadcastss xmm2, xmm2
	movups xmmword ptr [rbp - sizeof word - sizeof xmmword * 2], xmm2

	mov ax, word ptr [rbp - sizeof word]
	call Cos
	movaps xmm1, xmm0

	movups xmm0, xmmword ptr [rbp - sizeof word - sizeof xmmword]
	movups xmm2, xmmword ptr [rbp - sizeof word - sizeof xmmword * 2]

	mulps xmm0, xmm2;multiply normalized vector 3 from input with sin value of angle from input

	insertps xmm0, xmm1, INSERT_X_TO_W

	mov rsp, rbp
	pop rbp
	ret
QuatAngleAxis endp

;TODO: test if this function works
;al: a value in the range [0:255] which represents where on the curve to look up. returns an approximate sin value in xmm0.
Sin proc
	mov cl, 255
	div cl
	shr ax, 8
	movzx eax, al
	lea rcx, sinLookupTable
	mov cl, byte ptr [rcx + rax]
	movzx ecx, cl
	cvtsi2ss xmm0, rcx
	divss xmm0, twoFiddyfahv
	ret
Sin endp

;TODO: test if this function works
;al: a value in the range [0:255] which represents where on the curve to look up. returns an approximate cos value in xmm0.
Cos proc
	movzx cx, al
	sub al, cl
	sub al, cl
	add al, PI_ON_4
	sub rsp, 20h
	call Sin
	add rsp, 20h
	ret
Cos endp
endif