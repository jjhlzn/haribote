[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api035.nas"]

		GLOBAL	_getpid

[SECTION .text]
  
_getpid:		; int getpid(); 
		MOV		EDX, 35
		INT		0x40
		RET
