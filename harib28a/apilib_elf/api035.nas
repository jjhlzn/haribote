BITS 32

		GLOBAL	_getpid

[SECTION .text]
  
_getpid:		; int getpid(); 
		MOV		EDX, 35
		INT		0x40
		RET
