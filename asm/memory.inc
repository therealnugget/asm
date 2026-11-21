ifndef memoryInc
.data
memoryInc equ 1
;serialization provides mutual exclusion.
HEAP_NO_SERIALIZE equ 1
HEAP_ZERO_MEMORY equ 8

.code
;;does not give mutual exclusion.
;rcx: qword allocation size. returns location of memory block allocated.
MemAlloc proc
	mov r8, rcx
	mov rcx, heapHandle
	mov rdx, HEAP_NO_SERIALIZE
	sub rsp, 32
	call HeapAlloc
	add rsp, 32
	ret
MemAlloc endp
;;zeroes memory. does not give mutual exclusion.
;rcx: qword allocation size. returns location of memory block allocated.
MemCalloc proc
	mov r8, rcx
	mov rcx, heapHandle
	mov rdx, HEAP_NO_SERIALIZE
	or rdx, HEAP_ZERO_MEMORY
	sub rsp, 32
	call HeapAlloc
	add rsp, 32
	ret
MemCalloc endp
;rcx: qword location to free. If the function succeeds, the return value is nonzero. If the function fails, the return value is zero. An application can call GetLastError for extended error information.
MemFree proc
	mov r8, rcx
	mov rcx, heapHandle
	mov rdx, HEAP_NO_SERIALIZE
	sub rsp, 20h
	call HeapFree
	add rsp, 20h
	ret
MemFree endp
;rcx: dst, dl: value, r8: count
MemSet proc
	mov r9, rcx
	mov rcx, r8

setMemHead:
	mov byte ptr [r9 + rcx * sizeof byte - sizeof byte], dl
	loop setMemHead

	ret
MemSet endp
;rcx: dst, rdx: src, r8: size
MemCopy proc
	mov r9, rcx
	mov rcx, r8

copyMemHead:
	mov r10, rdx
	add r10, rcx
	dec r10
	mov r10b, byte ptr [r10]
	mov byte ptr [r9 + rcx * sizeof byte - sizeof byte], r10b
	loop copyMemHead

	ret
MemCopy endp
endif