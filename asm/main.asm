include highestInc.inc
include quaternions.inc
include input.inc
include renderer.inc
extern GetStdHandle: proc
extern GetProcessHeap: proc
extern WriteConsoleA: proc
extern ReadConsoleA: proc
extern ExitProcess: proc
extern HeapAlloc: proc
extern HeapFree: proc
extern Sleep: proc
extern WriteConsoleOutputA: proc
extern WriteConsoleOutputCharacterA: proc
extern SetConsoleWindowInfo: proc
extern SetConsoleScreenBufferSize: proc
extern DebugBreak: proc
extern GetLastError: proc
extern GetLargestConsoleWindowSize: proc
extern GetConsoleScreenBufferInfo: proc
extern SetConsoleDisplayMode: proc
extern GetConsoleWindow: proc
extern ShowWindow: proc
.data
CONSOLE_FULLSCREEN_MODE equ 1
CONSOLE_WINDOWED_MODE equ 2
INVALID_HANDLE_VALUE equ -1
TRUE equ 1
FALSE equ 0
HEAP_GENERATE_EXCEPTIONS equ 4
;serialization provides mutual exclusion.
HEAP_NO_SERIALIZE equ 1
HEAP_ZERO_MEMORY equ 8
STD_OUTPUT_HANDLE equ -11
small_rect_size equ sizeof SMALL_RECT
bits_in_byte equ 8
SW_MAXIMIZE equ 3

COORD struct
	X dw 0
	Y dw 0
COORD ends
SMALL_RECT struct
	Left dw ?
	Top dw ?
	Right dw ?
	Bottom dw ?
SMALL_RECT ends
Char union
	;;wide char
	UnicodeChar dw ?
	;;char
	AsciiChar db ?
	Char ends
	CHAR_INFO struct
	charUnion Char {0}
	Attributes dw ?
CHAR_INFO ends
CONSOLE_SCREEN_BUFFER_INFO struct
	dwSize COORD {?, ?}
	dwCursorPosition COORD {?, ?}
	wAttributes dw ?
	srWindow SMALL_RECT {?}
	dwMaximumWindowSize COORD {?, ?}
CONSOLE_SCREEN_BUFFER_INFO ends
CONSOLE_SCREEN_BUFFER_INFOEX struct
	cbSize dd ?
	dwSize COORD {?}
	dwCursorPosition COORD {?}
	wAttributes dw ?
	srWindow SMALL_RECT {?}
	dwMaximumWindowSize COORD {?}
	wPopupAttributes dw ?
	bFullScreenSupported db ?
	ColorTable dd ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
CONSOLE_SCREEN_BUFFER_INFOEX ends

charBuffer dq ?
stdOutHandle dq ?
heapHandle dq ?
charBufferMaxVal SMALL_RECT {?, ?, ?, ?}
charBufferLen dd ?
maxCharValue dd ?
SingleDefVec cubeScaleVec, 1.0
SingleDefVec squarePositionX, 0.0
SingleDefVec squarePositionY, 0.0
SingleDefVec squarePositionZ, -5.0
SingleDefVec squarePositionRotXX, 0.0
SingleDefVec squarePositionRotXY, 0.0
SingleDefVec squarePositionRotXZ, 0.0
SingleDefVec squarePositionRotYX, 0.0
SingleDefVec squarePositionRotYY, 0.0
SingleDefVec squarePositionRotYZ, 0.0
moveSpeed real4 0.1
XAxis real4 1.0, 0.0, 0.0, 0.0
YAxis real4 0.0, 1.0, 0.0, 0.0
charBufferSize COORD {?, ?}
bufferCoordOrigin COORD {0, 0}
isRotX db ?
isRotY db ?
rotXForPos db 0
rotYForPos db 0
.code

;takes input from posKey and negKey, subtracts (stack push/poop), stores it in xmm0, multiplies by moveSpeed then broadcasts to ymmReg
ConvInputToVec macro posKey, negKey
	mov ecx, posKey
	call GetKey
	mov ebx, eax
	push rbx
	mov ecx, negKey
	call GetKey
	pop rbx
	sub ebx, eax

	cvtsi2ss xmm0, ebx
	mulss xmm0, moveSpeed
	vbroadcastss ymm0, xmm0
endm

;uses xmm0. mov scalar to scalar with memory
MovS2SMem macro memDst, memSrc
	movss xmm0, memSrc
	movss memDst, xmm0
endm

;uses ymm0. mov vector to vector with memory
MovV2VMem macro memDst, memSrc
	movups xmm0, memSrc
	movups memDst, xmm0
endm

;takes input from posKey and negKey, subtracts (stack push/poop), stores it in bl.
ConvInputToScalar macro posKey, negKey, rotation, axis, keyBool, rotationForPos
	mov ecx, posKey
	call GetKey
	mov ebx, eax
	push rbx
	sub rsp, 28h
	mov ecx, negKey
	call GetKey
	add rsp, 28h
	pop rbx
	mov byte ptr [keyBool], bl
	or byte ptr [keyBool], al
	shl bl, 1
	shl al, 1
	sub bl, al
	mov byte ptr [rotationForPos], bl
	add byte ptr [rotation], bl
endm

;takes input from ymm0, adds to destVec in ymm1, stores in memory in destVec
LoadValFromInp macro destVec
	vaddps ymm0, ymm0, ymmword ptr [destVec]
	vmovups ymmword ptr [destVec], ymm0
endm

CalcRot macro rotation, axis
	mov al, byte ptr [rotation]
	movups xmm0, xmmword ptr [axis]
	call QuatAngleAxis
endm

LoadRotPosAxis macro pos
	vbroadcastss ymm1, xmm1

	vmovups ymmword ptr [pos], ymm1
endm

main proc

	push rbx
	push rbp
	mov rbp, rsp

	;the sizeof xmmword on the end is to calculate the cube's position rotation for the x axis, so that when that of the y is calculated, the x can be loaded back in before the quaternions are multiplied.
	sub rsp, (16 + 32) + sizeof xmmword;32 bytes of shadow stack space + 8 bytes for the fifth argument + another 8 bytes to keep func stack aligned by 16 bytes
	
	call GetProcessHeap
	mov heapHandle, rax

	mov rcx, STD_OUTPUT_HANDLE; set up first argument for GetStdHandle
	call GetStdHandle; get the handle to the console
	mov stdOutHandle, rax; store the handle in stdOutHandle
	
	cmp rax, INVALID_HANDLE_VALUE
	jne noStdHandleError
	call DebugBreak
noStdHandleError:

	call GetConsoleWindow
	mov rcx, rax
	mov rdx, SW_MAXIMIZE
	call ShowWindow

	sub rsp, sizeof CONSOLE_SCREEN_BUFFER_INFO
	mov rcx, stdOutHandle
	mov rdx, rsp
	call GetConsoleScreenBufferInfo

	mov eax, COORD ptr [rsp + sizeof CONSOLE_SCREEN_BUFFER_INFO - sizeof COORD]
	add rsp, sizeof CONSOLE_SCREEN_BUFFER_INFO
	mov charBufferSize, eax

	xor eax, eax
	mov ax, charBufferSize.X
	mov cx, charBufferSize.Y
	mul cx
	mov charBufferLen, eax
	dec eax
	mov maxCharValue, eax

	mov ecx, eax
	call MemAlloc
	mov charBuffer, rax

	mov ax, charBufferSize.X
	dec ax
	mov charBufferMaxVal.Right, ax
	mov ax, charBufferSize.Y
	dec ax
	mov charBufferMaxVal.Bottom, ax

	mov rcx, stdOutHandle
	mov edx, charBufferSize
	call SetConsoleScreenBufferSize

	cmp rax, 0
	jnz noErrorConsoleBufferSize
	call DebugBreak
noErrorConsoleBufferSize:

	mov rcx, stdOutHandle
	mov rdx, TRUE
	lea r8, charBufferMaxVal
	call SetConsoleWindowInfo

	cmp rax, 0
	jnz noConsoleWindowInfoError
	call DebugBreak
	
noConsoleWindowInfoError:

	sub rsp, KEYCODE_MAX
	lea rax, [rbp - KEYCODE_MAX]
	mov pressedKeys, rax

mainLoopHead:
		mov ecx, KEYCODE_CONTROL
		call GetKey
		mov bl, al
		mov ecx, KEYCODE_Q
		call GetKeyDown
		and al, bl
		jnz afterMainLoop

		mov rax, charBuffer
		mov ecx, charBufferLen
		xor rdx, rdx
		mov dl, '.'

	charAssignHead:
		mov byte ptr [rax + rcx * sizeof byte - sizeof byte], dl
		loop charAssignHead

		ConvInputToVec KEYCODE_D, KEYCODE_A
		LoadValFromInp squarePositionX

		ConvInputToVec KEYCODE_S, KEYCODE_W
		LoadValFromInp squarePositionY

		ConvInputToVec KEYCODE_Q, KEYCODE_E
		LoadValFromInp squarePositionZ

		ConvInputToScalar KEYCODE_LEFT, KEYCODE_RIGHT, rotationY, YAxis, isRotY, rotYForPos
		ConvInputToScalar KEYCODE_DOWN, KEYCODE_UP, rotationX, XAxis, isRotX, rotXForPos

		cmp byte ptr [isRotX], 0
		jz fromNoX

		CalcRot rotXForPos, XAxis
		movups [rbp - KEYCODE_MAX - sizeof xmmword], xmm0

		cmp byte ptr [isRotY], 0
		jz rotation
		jmp bothRot
	fromNoX:
		cmp byte ptr [isRotY], 0
		jz noRotation

	checkYRot:
		CalcRot rotYForPos, YAxis
		jmp rotation
	bothRot:
		CalcRot rotYForPos, YAxis
		movups xmm1, [rbp - KEYCODE_MAX - sizeof xmmword]
		call QuatMult

	rotation:
		movd xmm1, dword ptr [squarePositionX]
		insertps xmm1, real4 ptr [squarePositionY], INSERT_0_TO_1
		insertps xmm1, real4 ptr [squarePositionZ], INSERT_0_TO_2
		call QuatMultVec

		movss xmm1, xmm0
		LoadRotPosAxis squarePositionX
		
		extractps eax, xmm0, 1
		movd xmm1, eax
		LoadRotPosAxis squarePositionY
		
		extractps eax, xmm0, 2
		movd xmm1, eax
		LoadRotPosAxis squarePositionZ

	noRotation:

		vmovups ymm0, ymmword ptr [cubeScaleVec]
		vmovups ymm1, ymmword ptr [squarePositionX]
		vmovups ymm2, ymmword ptr [squarePositionY]
		vmovups ymm3, ymmword ptr [squarePositionZ]
		call RenderCubeLocScale

		mov rcx, stdOutHandle
		mov rdx, charBuffer
		mov r8d, charBufferLen
		mov r9d, bufferCoordOrigin
		sub rsp, 16
		lea rax, [rsp + 32 + 8]
		mov [rsp + 32], rax
		call WriteConsoleOutputCharacterA
		add rsp, 16
	
		mov rbx, KEYCODE_MAX
	pastKeyLoop:
		mov rcx, rbx
		call GetKey
		mov rcx, pressedKeys
		mov byte ptr [rcx + rbx], al
		cmp rbx, 0
		dec rbx
		jnz pastKeyLoop

		mov ecx, 1
		call Sleep
	jmp mainLoopHead
afterMainLoop:
	mov rcx, charBuffer
	call MemFree
	mov rsp, rbp
	pop rbp;just good convention to pop base ptr and restore stack ptr
	pop rbx
	mov rcx, rax; pass non-error arg to ExitProcess(int)
	sub rsp, 8
	call ExitProcess
	ret
main endp
;;does not give mutual exclusion.
;allocSize: qword
MemAlloc proc
	mov r8, rcx
	mov rcx, heapHandle
	mov rdx, HEAP_NO_SERIALIZE
	sub rsp, 32
	call HeapAlloc
	add rsp, 32
	ret
MemAlloc endp
;lpMem: qword
MemFree proc
	mov r8, rcx
	mov rcx, heapHandle
	mov rdx, HEAP_NO_SERIALIZE
	sub rsp, 20h
	call HeapFree
	add rsp, 20h
	ret
MemFree endp

end