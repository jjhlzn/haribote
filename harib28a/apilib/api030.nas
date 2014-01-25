[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api030.nas"]

		GLOBAL	_api_close

[SECTION .text]
  
_api_close:		; int api_close(int fd);
		PUSH EBP
		MOV 	EAX, [ESP+8] ;fd 
		MOV		EDX, 30
		INT		0x40
		POP EBP
		RET
