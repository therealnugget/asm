ifndef quaternionInc
.data
quaternionInc equ 1
QUATERNION_DOT_MASK equ 0f1h
VECTOR_DOT_MASK equ 0e1h
;;i.e. highest to lowest
W_TO_X_MASK equ 3h
SHUFFLE_CROSS_X_0 equ 201
SHUFFLE_CROSS_Y_0 equ 210
SHUFFLE_CROSS_X_1 equ 210
SHUFFLE_CROSS_Y_1 equ 210

SingleDefVec negOneVec, -1.0
SingleDefVec twoVec, 2.0
SingleDefVec negTwoVec, -2.0
QUATERNION_W_EXTRACTION dword 0, 0, 0, -1
QUATERNION_XYZ_EXTRACTION dword -1, -1, -1, 0

Quaternion struct
	real4 x 0.0
	real4 y 0.0
	real4 z 0.0
	real4 w 1.0
Quaternion ends
.code
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
	rsqrtss xmm1, xmm0
	vbroadcastss xmm1, xmm1
	mulps xmm0, xmm1
	ret
QuatNormalize endp

;xmm0: quaternion a, xmm1: quaternion b. returns the quaternion multiplication.
QuatMult proc
	vpshufd xmm2, xmm0, W_TO_X_MASK
	vbroadcastss xmm2, xmm2
	movaps xmm3, xmm1
	mulps xmm2, xmm3

	pshufd xmm2, xmm0, W_TO_X_MASK
	vbroadcastss xmm2, xmm2
	movaps xmm3, xmm0
	mulps xmm3, xmm2
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
	addps xmm0, xmm3

	;all bits higher than least significant single-precision floating point value are zeroed
	vdpps xmm2, xmm4, xmm5, VECTOR_DOT_MASK
	pshufd xmm3, xmm4, W_TO_X_MASK
	pshufd xmm1, xmm5, W_TO_X_MASK
	mulss xmm1, xmm3
	subss xmm1, xmm2

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

;xmm0: quaternion to conjugate
QuatConj proc
	vpand xmm1, xmm0, QUATERNION_XYZ_EXTRACTION
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
	vpand xmm1, xmm0, QUATERNION_XYZ_EXTRACTION
	mulps xmm1, negTwoVec
	addps xmm1, xmm0

	QuatLengthSqr xmm0
	divps xmm1, xmm0
	movaps xmm0, xmm1
	ret
QuatInv endp

;xmm0: quaternion, xmm1: vector3. returns a vector in xmm0 which represents the quaternion arg multiplied by the vector arg. bits [127:96] is an undefined arbitrary value
QuatMultVec proc
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
	mulps xmm0, twoVec

	mulps xmm3, xmm0
	addps xmm3, xmm4
	
	movaps xmm1, xmm0
	movaps xmm0, xmm4
	call VecCross
	addps xmm0, xmm3
	ret
QuatMultVec endp

QuatAngle proc
	
QuatAngle endp
endif