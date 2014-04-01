BITS 32

		GLOBAL	_api_read

[SECTION .text]
  
_api_read:		; int api_read(const int fd, const char * buf, const int len);
		PUSH EBP
		PUSH EBX
		MOV 	EAX, [ESP+12] ;fd 
		MOV     EBX, [ESP+16] ;buf
		MOV     EBP, [ESP+20] ;len
		MOV		EDX, 31
		INT		0x40
		POP EBX
		POP EBP
		RET
