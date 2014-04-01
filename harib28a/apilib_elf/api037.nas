BITS 32

		GLOBAL	_execv

[SECTION .text]
  
_execv:		; int execv(const char *path, char * argv[]); 
		PUSH 	EBX
		PUSH    ECX
		MOV  	EBX, [ESP+12]  ;path
		MOV     ECX, [ESP+16]  ;argv
		MOV		EDX, 37
		INT		0x40
		POP 	ECX
		POP     EBX
		RET
