BITS 32

		GLOBAL	_api_open

[SECTION .text]
  
_api_open:		; int api_open(const char *pathname, int flags);
		PUSH EBX
		MOV 	EAX, [ESP+8] ;pathname 
		MOV     EBX, [ESP+12] ;flags
		MOV		EDX, 29
		INT		0x40
		POP EBX
		RET
