BITS 32

		GLOBAL	_fork

[SECTION .text]
  
_fork:		; int fork(); 
		MOV		EDX, 34
		INT		0x40
		RET
