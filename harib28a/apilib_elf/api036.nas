BITS 32

		GLOBAL	_wait

[SECTION .text]
  
_wait:		; int wait(int *status); 
		PUSH 	EBX
		MOV  	EBX, [ESP+8]
		MOV		EDX, 36
		INT		0x40
		POP 	EBX
		RET
