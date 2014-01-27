[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api032.nas"]

		GLOBAL	_api_write

[SECTION .text]
  
_api_write:		; int api_write(const int fd, const char * buf, const int len); 
		PUSH EBP
		PUSH EBX
		MOV 	EAX, [ESP+12] ;fd 
		MOV     EBX, [ESP+16] ;buf
		MOV     EBP, [ESP+20] ;len
		MOV		EDX, 32
		INT		0x40
		POP EBX
		POP EBP
		RET
