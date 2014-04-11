BITS 32

		GLOBAL	write

[SECTION .text]

write:	; ssize_t write(int fd, const void *buf, size_t count);
		PUSH EBX
		PUSH ECX
		MOV 	EBX, [ESP+12] ;fd 
		MOV     ECX, [ESP+16] ;buf
		MOV     EDX, [ESP+20] ;len
		MOV		EAX, 4
		INT		0x80
		POP ECX
		POP EBX
		RET
