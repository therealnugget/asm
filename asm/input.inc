ifndef input
input equ 1
extern GetKeyState: proc
.data
KEYCODE_A equ 41h
KEYCODE_D equ 44h
KEYCODE_S equ 53h
KEYCODE_K equ 04bh
KEYCODE_W equ 57h
KEYCODE_CONTROL equ 0a2h
KEYCODE_Q equ 51h
KEYCODE_E equ 45h
KEYCODE_RIGHT equ 27h
KEYCODE_LEFT equ 25h
KEYCODE_UP equ 26h
KEYCODE_DOWN equ 28h
;				   0b1000 0000 0000 0000
KEY_DOWN_STATE equ 8000h
KEY_DOWN_BIT_INDEX equ 15
KEYCODE_MAX equ 176

pressedKeys dq ?

.code
;key: dword
GetKey proc
	sub rsp, 20h
	call GetKeyState
	add rsp, 20h
	and eax, KEY_DOWN_STATE
	shr eax, KEY_DOWN_BIT_INDEX
	ret
GetKey endp
;key: dword
GetPastKey proc
	mov rdx, qword ptr [pressedKeys]
	mov al, byte ptr [rdx + rcx]
	ret
GetPastKey endp
;rcx: key, dword
GetKeyDown proc
	push rbp
	mov rbp, rsp
	sub rsp, 32
	push rcx
	call GetKey
	mov dl, al
	pop rcx
	push rdx
	call GetPastKey
	pop rdx
	not al
	and al, dl
	mov al, dl
	mov rsp, rbp
	pop rbp
	ret
GetKeyDown endp
endif