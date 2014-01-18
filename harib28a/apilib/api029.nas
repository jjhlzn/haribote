[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api029.nas"]

		GLOBAL	_api_open

[SECTION .text]
  
_api_open:		; int api_open(const char *pathname, int flags);
		PUSH EBP
		MOV 	EAX, [ESP+4] ;flags
		MOV     EBP, [ESP+8] ;pathname
		MOV		EDX, 29
		INT		0x40
		POP EBP
		RET
