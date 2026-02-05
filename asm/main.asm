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
moveSpeed real4 0.05
origRotation real4 1.0, 2.0, 3.0, 4.0
tempRotation real4 5.0, 6.0, 7.0, 8.0
charBufferSize COORD {?, ?}
bufferCoordOrigin COORD {0, 0}
;;bool[KEYCODE_MAX]
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

;takes input from ymm0, adds to positionVec in ymm1, stores in memory in positionVec
LoadPosFromInp macro positionVec
	vmovups ymm1, ymmword ptr [positionVec]
	vaddps ymm0, ymm0, ymm1
	vmovups ymmword ptr [positionVec], ymm0
endm

main proc

	push rbx
	push rbp
	mov rbp, rsp

	sub rsp, (16 + 32);32 bytes of shadow stack space + 8 bytes for the fifth argument + another 8 bytes to keep func stack aligned by 16 bytes
	
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

		ConvInputToVec KEYCODE_Q, KEYCODE_E
		LoadPosFromInp squarePositionZ

		ConvInputToVec KEYCODE_D, KEYCODE_A
		LoadPosFromInp squarePositionX

		ConvInputToVec KEYCODE_S, KEYCODE_W
		LoadPosFromInp squarePOsitionY

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
	xor rax, rax; return 0
	mov rcx, rax; pass non-error arg to ExitProcess(int)
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