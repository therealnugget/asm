ifndef input
input equ 1
extern GetKeyState: proc
.data
KEYCODE_A equ 41h
KEYCODE_D equ 44h
KEYCODE_S equ 53h
KEYCODE_W equ 57h
KEYCODE_CONTROL equ 17
KEYCODE_Q equ 51h
KEYCODE_E equ 45h
;				   0b1000 0000 0000 0000
KEY_DOWN_STATE equ 8000h
KEY_DOWN_BIT_INDEX equ 15
KEYCODE_MAX equ 176

pressedKeys dq ?
.code
GetKey proc key: dword
	sub rsp, 20h
	call GetKeyState
	add rsp, 20h
	and eax, KEY_DOWN_STATE
	shr eax, KEY_DOWN_BIT_INDEX
	ret
GetKey endp

GetPastKey proc key: dword
	mov rdx, pressedKeys
	mov al, byte ptr [rdx + rcx]
	ret
GetPastKey endp

GetKeyDown proc key: dword
	mov [rbp - 8], rcx
	sub rsp, 40
	push rbx
	call GetKey
	mov bl, al
	;don't forget rbx was pushed!!!
	mov rcx, [rbp - 8]
	call GetPastKey
	;notting the lsb
	xor al, 1
	and al, bl
	pop rbx
	add rsp, 40
	ret
GetKeyDown endp
endif