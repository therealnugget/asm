ifndef quaternionInc
.data
quaternionInc equ 1
QUATERNION_DOT_MASK equ 0f1h
VECTOR_DOT_MASK equ 071h
VECTOR_2_DOT_MASK equ 031h
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
LESS_EQU_VEC_CMP equ 2h
LESS_VEC_CMP equ 1h
GREATER_VEC_CMP equ 6h
NOT_EQU_VEC_CMP equ 4h
EQU_VEC_CMP equ 0h
PI_ON_4 equ 63

SingleDefVec negOneVec, -1.0
SingleDefVec twoVec, 2.0
SingleDefVec negTwoVec, -2.0
MAX_SIN_AMP real4 127.0
one real4 1.0
zero real4 0.0
negTwo real4 -2.0
two real4 2.0
negOne real4 -1.0
acos_iteration_0 real4 -0.0187293
acos_iteration_1 real4 0.0742610
acos_iteration_2 real4 -0.2121144
acos_iteration_3 real4 1.5707288
forward_vector3 real4 0.0, 0.0, 1.0, 0.0
pi real4 3.14159265358979
align 16
ABS_VALUE_MASK_F dd 7FFFFFFFh, 7FFFFFFFh, 7FFFFFFFh, 7FFFFFFFh
QUATERNION_W_EXTRACTION dd 0, 0, 0, -1
QUATERNION_XYZ_EXTRACTION dd -1, -1, -1, 0

sinLookupTable db 0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 34, 37, 39, 42, 45, 48, 51, 54, 57, 60, 62, 65, 68, 70, 73
db 75, 78, 80, 83, 85, 87, 90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 110, 112, 113, 115, 116, 117, 118, 120
db 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127, 126, 126, 126, 125, 125, 124
db 123, 122, 122, 121, 120, 118, 117, 116, 115, 113, 112, 110, 109, 107, 106, 104, 102, 100, 98, 96, 94, 92, 90, 87, 85
db 83, 80, 78, 75, 73, 70, 68, 65, 62, 60, 57, 54, 51, 48, 45, 42, 39, 37, 34, 30, 27, 24, 21, 18, 15, 12, 9, 6, 3, 0
db -4, -7, -10, -13, -16, -19, -22, -25, -28, -31, -35, -38, -40, -43, -46, -49, -52, -55, -58, -61, -63, -66, -69, -71
db -74, -76, -79, -81, -84, -86, -88, -91, -93, -95, -97, -99, -101, -103, -105, -107, -108, -110, -111, -113, -114, -116
db -117, -118, -119, -121, -122, -123, -123, -124, -125, -126, -126, -127, -127, -127, -128, -128, -128, -128, -128, -128
db -128, -127, -127, -127, -126, -126, -125, -124, -123, -123, -122, -121, -119, -118, -117, -116, -114, -113, -111, -110
db -108, -107, -105, -103, -101, -99, -97, -95, -93, -91, -88, -86, -84, -81, -79, -76, -74, -71, -69, -66, -63, -61, -58
db -55, -52, -49, -46, -43, -40, -38, -35, -31, -28, -25, -22, -19, -16, -13, -10, -7, -4

Quaternion struct
	real4 x 0.0
	real4 y 0.0
	real4 z 0.0
	real4 w 1.0
Quaternion ends

.code
;xmm0: angle in radians. returns arccos in xmm0
Acos proc
	movaps xmm2, xmm0
	pand xmm0, xmmword ptr [ABS_VALUE_MASK_F]
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
	vcmpss xmm2, xmm0, zero, GREATER_VEC_CMP
	vbroadcastss xmm2, xmm2
	rsqrtss xmm1, xmm0
	vbroadcastss xmm1, xmm1
	mulps xmm0, xmm1
	pand xmm0, xmm2
	ret
QuatNormalize endp

;xmm0: quaternion a, xmm1: quaternion b. returns the quaternion multiplication in xmm0.
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

	;;TODO: make this more efficient
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
	insertps xmm0, xmm1, INSERT_X_TO_W

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
	cmpss xmm1, zero, GREATER_VEC_CMP
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
	vcmpss xmm3, xmm0, zero, GREATER_VEC_CMP
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
	movzx ax, al
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

;al: range[0:255] representing x rotation, cl: range[0:255] representing y rotation
EulerToQuat proc
	;let: phi = 0.0
	;let: theta = cl
	;let: z-rotation = al
	push rbp
	mov rbp, rsp
	sub rsp, sizeof xmmword * 3
	shr al, 1
	movzx ax, al
	push rcx
	push rax
	sub rsp, 32
	call sin
	add rsp, 32
	pop rax
	movups xmmword ptr [rbp - sizeof xmmword * 1], xmm0;	xmm1: sin(al / 2)
	sub rsp, 32
	call cos
	add rsp, 32
	movups xmmword ptr [rbp - sizeof xmmword * 2], xmm0;	xmm2: cos(al / 2)
	;purposely duplicating a bit of code here to make it more readable because realistically you don't want a proc here and you can't step into a macro
	pop rcx
	mov al, cl
	shr al, 1
	movzx ax, al
	push rax
	sub rsp, 32
	call sin
	add rsp, 32
	movups xmmword ptr [rbp - sizeof xmmword * 3], xmm0;	xmm3: sin(cl / 2)
	pop rax
	sub rsp, 32
	call cos
	add rsp, 32
	movups xmm1, xmmword ptr [rbp - sizeof xmmword * 1]
	movups xmm2, xmmword ptr [rbp - sizeof xmmword * 2]
	movups xmm3, xmmword ptr [rbp - sizeof xmmword * 3]
	mov rsp, rbp
	pop rbp
	movaps xmm4, xmm0;										xmm4: cos(cl / 2)
	mulss xmm0, xmm2;now the lower single-precision floating point value of xmm0 is cos(al/2)cos(cl/2). multiplying like this to save instructions because obviously cos(cl/2), xmm4, is left in xmm0
	movaps xmm5, xmm1
	mulss xmm5, negOne
	mulss xmm5, xmm3
	insertps xmm0, xmm5, INSERT_0_TO_1;now the second single-precision floating point value of xmm0 is -sin(cl/2)sin(al/2)
	movaps xmm5, xmm2
	mulss xmm5, xmm3
	insertps xmm0, xmm5, INSERT_0_TO_2; now the third single-precision floating point value of xmm0 is sin(cl/2)cos(al/2)
	mulss xmm1, xmm4
	insertps xmm0, xmm1, INSERT_0_TO_3; now the highest single-precision floating point value of xmm0 is -sin(al/2)sin(cl/2)
	ret
	;https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
EulerToQuat endp

;al: a value in the range [0:255] which represents where on the curve to look up. returns an approximate sin value in xmm0.
Sin proc
	mov cl, 255
	div cl
	shr ax, 8
	movzx eax, al
	lea rcx, sinLookupTable
	mov cl, byte ptr [rcx + rax]
	movsx ecx, cl
	cvtsi2ss xmm0, ecx
	divss xmm0, MAX_SIN_AMP
	ret
Sin endp

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