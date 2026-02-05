ifndef highestInc
.data
highestInc equ 1
.code
;single defines vector
SingleDefVec macro vecName, value
	vecName real4 value, value, value, value, value, value, value, value
endm
;single defines xmm vector
SingleDefVecX macro vecName, value
	vecName real4 value, value, value, value
endm

LoadSinglesXmm macro xmmReg, val1, val2, val3, val4
	
endm
endif