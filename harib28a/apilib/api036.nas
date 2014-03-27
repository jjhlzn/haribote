[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api036.nas"]

		GLOBAL	_wait

[SECTION .text]
  
_wait:		; int wait(int *status); 
		PUSH 	EBX
		MOV  	EBX, [ESP+8]
		MOV		EDX, 35
		INT		0x40
		POP 	EBX
		RET
