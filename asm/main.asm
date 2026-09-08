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
extern QueryPerformanceCounter: proc
extern QueryPerformanceFrequency: proc
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
NUM_BITS_IN_WORD equ sizeof word * bits_in_byte
NUM_BITS_IN_DWORD equ sizeof dword * bits_in_byte
TIMER_QUADPART_MULTIPLICAND equ 1000000
FRAME_WAIT_TIME equ 1

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
DUMMYSTRUCTNAME_LARGE_INTEGER_ struct
	LowPart dd ?
	HighPart dd ?
DUMMYSTRUCTNAME_LARGE_INTEGER_ ends
LARGE_INTEGER union
	dummyStruct DUMMYSTRUCTNAME_LARGE_INTEGER_ {?, ?}
	u DUMMYSTRUCTNAME_LARGE_INTEGER_ {?, ?}
	QuadPart dq ?
LARGE_INTEGER ends

charBuffer dq ?
stdOutHandle dq ?
heapHandle dq ?
charBufferMaxVal SMALL_RECT {?, ?, ?, ?}
charBufferLen dd ?
maxCharValue dd ?
SingleDefVec cubeScaleVec, 1.0
SingleDefVec cameraInputX, 0.0
SingleDefVec cameraInputY, 0.0
SingleDefVec cameraInputZ, 0.0
moveSpeed real4 10.0
rotationSpeed real4 270.0
timerQuadpartSecondDivisor real4 1000000.0
frequency LARGE_INTEGER {{?, ?}}
pTime LARGE_INTEGER {{?, ?}}
cTime LARGE_INTEGER {{?, ?}}
XAxis real4 1.0, 0.0, 0.0, 0.0
YAxis real4 0.0, 1.0, 0.0, 0.0
ZAxis real4 0.0, 0.0, 1.0, 0.0
charBufferSize COORD {?, ?}
bufferCoordOrigin COORD {0, 0}
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
	push rbx
	sub rsp, 20h
	call DeltaTime
	add rsp, 20h
	pop rbx
	cvtsi2ss xmm1, ebx
	mulss xmm1, moveSpeed
	mulss xmm1, xmm0
	vbroadcastss ymm1, xmm1
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
ConvInputToScalar macro posKey, negKey, rotation
	mov ecx, posKey
	sub rsp, 20h
	call GetKey
	add rsp, 20h
	mov ebx, eax
	push rbx
	sub rsp, 20h
	mov ecx, negKey
	call GetKey
	add rsp, 20h
	pop rbx
	sub bl, al
	push rbx
	sub rsp, 20h
	call DeltaTime
	add rsp, 20h
	pop rbx
	movsx rbx, bl
	cvtsi2ss xmm1, rbx
	mulss xmm1, xmm0
	mulss xmm1, real4 ptr[rotationSpeed]
	cvtss2si rbx, xmm1
	mov byte ptr [rotation], bl
endm

CalcRot macro rotation, axis
	mov al, byte ptr [rotation]
	movups xmm0, xmmword ptr [axis]
	call QuatAngleAxis
endm

;dst: destination register, val: value to shift in, shf: imm8 value to shift by
ShiftIntoReg macro dst, val, shf
	or dst, val
	shl dst, shf
endm

DeltaTime proc
	lea rcx, cTime
	sub rsp, 20h
	call QueryPerformanceCounter
	add rsp, 20h
	mov rax, qword ptr [cTime]
	sub rax, qword ptr [pTime]
	mov rdx, TIMER_QUADPART_MULTIPLICAND
	mul rdx
	cvtsi2ss xmm0, rax
	cvtsi2ss xmm1, qword ptr [frequency]
	divss xmm0, xmm1
	movss xmm1, timerQuadpartSecondDivisor
	divss xmm0, xmm1
	ret
DeltaTime endp

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

	vmovups ymm4, ymmword ptr [cubeVerticesX]
	vmovups ymm5, ymmword ptr [cubeVerticesY]
	vmovups ymm6, ymmword ptr [cubeVerticesZ]
	vmovups ymm0, ymmword ptr [cubeScaleVec]
	vmulps ymm4, ymm4, ymm0
	vmulps ymm5, ymm5, ymm0
	vmulps ymm6, ymm6, ymm0

	vaddps ymm6, ymm6, ymmword ptr [verticesZOffset]

	vmovups ymmword ptr [cubeVerticesX], ymm4
	vmovups ymmword ptr [cubeVerticesY], ymm5
	vmovups ymmword ptr [cubeVerticesZ], ymm6

	lea rcx, frequency
	call QueryPerformanceFrequency
	lea rcx, pTime
	call QueryPerformanceCounter

mainLoopHead:
		
		mov ecx, KEYCODE_CONTROL
		call GetKey
		mov bl, al
		push rbx
		mov ecx, KEYCODE_Q
		call GetKeyDown
		pop rbx
		and al, bl
		jnz afterMainLoop

		mov rax, charBuffer
		mov ecx, charBufferLen
		xor rdx, rdx
		mov dl, "'"

	charAssignHead:
		mov byte ptr [rax + rcx * sizeof byte - sizeof byte], dl
		loop charAssignHead

		ConvInputToVec KEYCODE_D, KEYCODE_A
		vmovups ymmword ptr [cameraInputX], ymm1

		ConvInputToVec KEYCODE_S, KEYCODE_W
		vmovups ymmword ptr [cameraInputY], ymm1

		ConvInputToVec KEYCODE_Q, KEYCODE_E
		vmovups ymmword ptr [cameraInputZ], ymm1

		ConvInputToScalar KEYCODE_LEFT, KEYCODE_RIGHT, rotationY
		ConvInputToScalar KEYCODE_DOWN, KEYCODE_UP, rotationX


		vmovups ymm0, ymmword ptr [cameraInputX]
		vmovups ymm1, ymmword ptr [cameraInputY]
		vmovups ymm2, ymmword ptr [cameraInputZ]
		call RenderCubeLoc

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

		lea rcx, pTime
		call QueryPerformanceCounter

		mov ecx, FRAME_WAIT_TIME
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