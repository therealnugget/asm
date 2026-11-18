ifndef highestInc
.data
highestInc equ 1
;maximum unsigned doubleword value AS a float
u32_max_f real4 4294967295.0

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