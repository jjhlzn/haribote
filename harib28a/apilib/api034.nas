[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api034.nas"]

		GLOBAL	_fork

[SECTION .text]
  
_fork:		; int fork(); 
		MOV		EDX, 34
		INT		0x40
		RET
