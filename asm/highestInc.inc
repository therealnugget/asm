ifndef highestInc
highestInc equ 1
.data
;single defines vector
SingleDefVec macro singleName, vecName, value
	singleName equ value
	vecName real4 singleName, singleName, singleName, singleName, singleName, singleName, singleName, singleName
endm
endif